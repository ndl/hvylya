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

#pragma once

#include <hvylya/pipelines/async/block.h>

#include <condition_variable>
#include <thread>

namespace hvylya {
namespace pipelines {
namespace async {

class Pipeline: core::NonCopyable
{
  public:
    Pipeline(std::size_t min_extra_queue_load = 0);

    void add(filters::IFilter& top_filter);

    void start();

    void pause();

    void reset();

    void resume();

    void stop();

    void wait();

    void run();

    void inputAvailableSizeChanged(Block& block, std::size_t input_channel);

    void outputAvailableSizeChanged(Block& block, std::size_t output_channel);

  private:
    enum class State: std::int8_t
    {
        Stopped,
        Paused,
        Running
    };

    std::vector<std::thread> threads_;
    std::deque<Block*> queue_;
    std::vector<Block> blocks_;
    std::condition_variable task_scheduled_;
    std::mutex tasks_queue_mutex_;
    std::size_t min_extra_queue_load_, threads_count_, threads_running_, threads_waiting_, threads_waking_up_;
    std::unique_ptr<core::ExceptionBase> last_exception_;
    bool scheduling_, relaxed_mode_, finished_, stalled_;
    State state_;

    bool trySchedulingBlock(Block& block);

    bool scheduleAllBlocks();

    void runThread();

    void finalize();
};

} // namespace async
} // namespace pipelines
} // namespace hvylya
