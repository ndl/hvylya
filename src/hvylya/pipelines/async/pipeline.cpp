// Hvylya - software-defined radio framework, see https://endl.ch/projects/hvylya
//
// Copyright (C) 2019 - 2020 Alexander Tsvyashchenko <sdr@endl.ch>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <hvylya/pipelines/async/pipeline.h>

#include <hvylya/filters/io_states.h>

using namespace hvylya::filters;
using namespace hvylya::pipelines::async;

namespace {

void addLinkedFilters(std::unordered_set<IFilter*>& filters, IFilter& filter)
{
    for (std::size_t i = 0; i < filter.inputChannelsCount(); ++i)
    {
        for (auto source: filter.sources(i))
        {
            IFilter& source_filter = std::get<0>(source);
            if (!filters.count(&source_filter))
            {
                filters.insert(&source_filter);
                addLinkedFilters(filters, source_filter);
            }
        }
    }

    for (std::size_t i = 0; i < filter.outputChannelsCount(); ++i)
    {
        for (auto sink: filter.sinks(i))
        {
            IFilter& sink_filter = std::get<0>(sink);
            if (!filters.count(&sink_filter))
            {
                filters.insert(&sink_filter);
                addLinkedFilters(filters, sink_filter);
            }
        }
    }
}

} // anonymous namespace

Pipeline::Pipeline(std::size_t min_extra_queue_load):
    min_extra_queue_load_(min_extra_queue_load),
    threads_count_(0),
    threads_running_(0),
    threads_waiting_(0),
    threads_waking_up_(0),
    state_(State::Stopped)
{
}

void Pipeline::add(IFilter& top_filter)
{
    CHECK_EQ(0, top_filter.inputChannelsCount());

    std::unordered_set<IFilter*> filters;
    std::unordered_map<IFilter*, Block*> blocks_map;

    filters.insert(&top_filter);
    addLinkedFilters(filters, top_filter);

    blocks_.reserve(filters.size());
    for (auto filter: filters)
    {
        blocks_.emplace_back(*this, *filter);
        blocks_map[filter] = &blocks_.back();
    }

    for (auto filter: filters)
    {
        CHECK(blocks_map.count(filter));
        Block& current_block = *blocks_map[filter];

        for (std::size_t i = 0; i < filter->inputChannelsCount(); ++i)
        {
            for (auto& source: filter->sources(i))
            {
                CHECK(blocks_map.count(&std::get<0>(source)));
                Block& source_block = *blocks_map[&std::get<0>(source)];
                current_block.reader(i).setWriter(source_block.writer(std::get<1>(source)));
            }
        }

        for (std::size_t i = 0; i < filter->outputChannelsCount(); ++i)
        {
            for (auto& sink: filter->sinks(i))
            {
                CHECK(blocks_map.count(&std::get<0>(sink)));
                Block& sink_block = *blocks_map[&std::get<0>(sink)];
                current_block.writer(i).addReader(sink_block.reader(std::get<1>(sink)));
            }
        }
    }
}

void Pipeline::start()
{
    CHECK(state_ == State::Stopped) << "Attempted to start pipeline in state = " << int(state_);

    last_exception_.reset();
    relaxed_mode_ = false;
    scheduling_ = false;
    finished_ = false;
    stalled_ = false;

    scheduleAllBlocks();

    state_ = State::Running;

    // Start enough threads - but not more than blocks we have,
    // as blocks cannot be executed simultaneously by multiple threads.
    threads_count_ =
        std::min<std::size_t>(
            std::thread::hardware_concurrency(),
            blocks_.size()
        );
    threads_.reserve(threads_count_);
    threads_running_ = threads_count_;
    for (unsigned i = 0; i < threads_count_; ++i)
    {
        threads_.emplace_back(&Pipeline::runThread, this);
    }
}

void Pipeline::pause()
{
    std::unique_lock<std::mutex> lock(tasks_queue_mutex_);

    if (state_ == State::Running)
    {
        state_ = State::Paused;
        threads_waiting_ = 0;
        task_scheduled_.notify_all();
    }
    else
    {
        LOG(FATAL) << "Attempted to pause non-running pipeline";
    }
}

void Pipeline::reset()
{
    std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
    CHECK(state_ == State::Paused) << "Attempted to reset the non-paused pipeline";

    for (auto& block: blocks_)
    {
        block.reset();
    }

    queue_.clear();
    lock.unlock();
    scheduleAllBlocks();
}

void Pipeline::resume()
{
    std::unique_lock<std::mutex> lock(tasks_queue_mutex_);

    if (state_ == State::Paused)
    {
        state_ = State::Running;
        threads_waiting_ = 0;
        task_scheduled_.notify_all();
    }
    else if (state_ == State::Stopped)
    {
        // We don't need the lock anymore - release it so that
        // the threads that start() creates can execute immediately.
        lock.unlock();
        start();
    }
    else
    {
        LOG(FATAL) << "Attempted to resume pipeline in state = " << int(state_);
    }
}

void Pipeline::wait()
{
    // Wait until all the threads are finished:
    for (auto& thread: threads_)
    {
        thread.join();
    }
}

void Pipeline::stop()
{
    {
        std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
        CHECK(state_ == State::Running || state_ == State::Paused) << "Attempted to stop pipeline in state = " << int(state_);
        state_ = State::Stopped;
        threads_waiting_ = 0;
        task_scheduled_.notify_all();
    }

    wait();
    finalize();
}

void Pipeline::finalize()
{
    if (last_exception_)
    {
        last_exception_->rethrow();
    }

    bool eof_found = false;
    for (auto& block: blocks_)
    {
        for (std::size_t i = 0; !eof_found && i < block.outputChannelsCount(); ++i)
        {
            eof_found = eof_found || block.outputState(i).eof();
        }

        CHECK(!finished_ || block.state().load() == Block::State::Idle) << block.internalState();
    }

    stalled_ = !eof_found;

    if (stalled_)
    {
        LOG(ERROR) << "Pipeline stalled with pending input data";
    }
}

void Pipeline::run()
{
    start();
    wait();
    finalize();
}

bool Pipeline::scheduleAllBlocks()
{
    bool scheduled = false;

    for (auto& block: blocks_)
    {
        scheduled = trySchedulingBlock(block) || scheduled;
    }

    return scheduled;
}

void Pipeline::runThread()
{
    while (true)
    {
        Block* block;
        {
            std::unique_lock<std::mutex> lock(tasks_queue_mutex_);

            --threads_running_;

            if (state_ == State::Stopped)
            {
                // Exit the loop.
                break;
            }

            if (queue_.empty() && !threads_running_ && !scheduling_)
            {
                if (!relaxed_mode_)
                {
                    // First time we're here - reschedule blocks in relaxed
                    // size mode to process the rest of the data.
                    relaxed_mode_ = true;
                    scheduling_ = true;

                    // Release lock because scheduling will try to acquire it.
                    lock.unlock();
                    scheduleAllBlocks();
                    lock.lock();

                    scheduling_ = false;
                    ++threads_running_;
                    continue;
                }
                else
                {
                    // We're in "empty queue" clause the second time - by now all blocks
                    // that can be possibly scheduled under relaxed scheme should have been
                    // scheduled and executed, including those that became schedulable after
                    // initial blocks under relaxed scheme were scheduled.
                    // Check this is the case indeed.
                    lock.unlock();
                    CHECK(!scheduleAllBlocks());
                    lock.lock();

                    // Switch into the 'stopped' mode.
                    state_ = State::Stopped;
                    finished_ = true;

                    // Let everybody know we're stalled and exit the loop.
                    threads_waiting_ = 0;
                    task_scheduled_.notify_all();
                    break;
                }
            }

            if (queue_.empty())
            {
                ++threads_waiting_;
                task_scheduled_.wait(
                    lock,
                    [=]
                    {
                        bool ready = state_ == State::Stopped;
                        if (threads_waking_up_)
                        {
                            ready = ready || !queue_.empty();
                            if (!ready)
                            {
                                ++threads_waiting_;
                            }

                            --threads_waking_up_;
                        }

                        return ready;
                    }
                );
            }
            else if (threads_waiting_ && queue_.size() > threads_count_ - threads_waiting_ + min_extra_queue_load_)
            {
                --threads_waiting_;
                ++threads_waking_up_;
                task_scheduled_.notify_one();
            }

            if (state_ == State::Stopped)
            {
                // Exit the loop.
                break;
            }
            else if (state_ == State::Paused)
            {
                task_scheduled_.wait(
                    lock,
                    [=]
                    {
                        return state_ != State::Paused;
                    }
                );
                ++threads_running_;
                continue;
            }

            block = queue_.front();
            queue_.pop_front();

            ++threads_running_;
        }

        CHECK(block->state().load() == Block::State::Scheduled);
        block->state().store(Block::State::Running);

        try
        {
            block->process();
        }
        catch (core::ExceptionBase& ex)
        {
            std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
            block->state().store(Block::State::Idle);
            if (!last_exception_)
            {
                last_exception_ = ex.clone();
                state_ = State::Stopped;
                threads_waiting_ = 0;
                task_scheduled_.notify_all();
            }
            break;
        }

        block->state().store(Block::State::Idle);

        trySchedulingBlock(*block);
    }
}

bool Pipeline::trySchedulingBlock(Block& block)
{
    Block::State expected_state = Block::State::Idle;
    while (!block.state().compare_exchange_weak(expected_state, Block::State::Scheduling))
    {
        if (expected_state == Block::State::Running || expected_state == Block::State::Scheduled)
        {
            // Nothing to do: the block is scheduled and / or running,
            // so it will see all changes without any further action
            // either when it's started or after execution is finished
            // and next scheduling is attempted.
            return false;
        }
        expected_state = Block::State::Idle;
    }

    // We've got access - see if we want to schedule this block.
    bool schedulable = true;

    for (std::size_t i = 0; schedulable && i < block.inputChannelsCount(); ++i)
    {
        const InputState& state = block.inputState(i);
        std::size_t available_size = block.reader(i).availableSize();
        bool relaxed_size = relaxed_mode_ || block.reader(i).wrapping();
        schedulable = available_size >= state.historySize() + (relaxed_size ? state.requiredSize() : state.suggestedSize());
    }

    for (std::size_t i = 0; schedulable && i < block.outputChannelsCount(); ++i)
    {
        const OutputState& state = block.outputState(i);
        std::size_t available_size = block.writer(i).availableSize();
        bool relaxed_size = relaxed_mode_ || block.writer(i).wrapping();
        schedulable = !state.eof() && available_size >= (relaxed_size ? state.requiredSize() : state.suggestedSize());
    }

    if (schedulable)
    {
        std::unique_lock<std::mutex> lock(tasks_queue_mutex_);
        queue_.push_back(&block);
        block.state().store(Block::State::Scheduled);
    }
    else
    {
        block.state().store(Block::State::Idle);
    }

    return schedulable;
}

void Pipeline::inputAvailableSizeChanged(Block& block, std::size_t /* input_channel */)
{
    trySchedulingBlock(block);
}

void Pipeline::outputAvailableSizeChanged(Block& block, std::size_t /* output_channel */)
{
    trySchedulingBlock(block);
}
