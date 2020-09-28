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

#include <hvylya/pipelines/async/block.h>

#include <hvylya/filters/io_states.h>

#include <sstream>

using namespace hvylya::core;
using namespace hvylya::filters;
using namespace hvylya::pipelines::async;

namespace {

const std::size_t DefaultBufferScale = 64;

} // anonymous namespace

Block::Block(Pipeline& pipeline, IFilter& filter):
    pipeline_(pipeline),
    filter_(filter),
    inputs_(filter_.inputChannelsCount()),
    outputs_(filter_.outputChannelsCount())
{
    state_.store(Block::State::Idle);

    for (std::size_t i = 0; i < filter_.inputChannelsCount(); ++i)
    {
        readers_.emplace_back(CircularBufferReader(pipeline_, *this, i));
    }

    for (std::size_t i = 0; i < filter_.outputChannelsCount(); ++i)
    {
        writers_.emplace_back(
            CircularBufferWriter(
                pipeline_,
                *this,
                i,
                DefaultBufferScale * filter_.outputState(i).suggestedSize()
            )
        );
    }
}

Block::Block(Block&& block):
    pipeline_(block.pipeline_),
    filter_(block.filter_),
    readers_(std::move(block.readers_)),
    writers_(std::move(block.writers_)),
    inputs_(std::move(block.inputs_)),
    outputs_(std::move(block.outputs_))
{
    state_.store(block.state_.load());
}

std::atomic<Block::State>& Block::state()
{
    return state_;
}

std::size_t Block::inputChannelsCount() const
{
    return filter_.inputChannelsCount();
}

std::size_t Block::outputChannelsCount() const
{
    return filter_.outputChannelsCount();
}

const InputState& Block::inputState(std::size_t input_channel) const
{
    CHECK_LT(input_channel, inputs_.size());
    return filter_.inputState(input_channel);
}

const OutputState& Block::outputState(std::size_t output_channel) const
{
    CHECK_LT(output_channel, outputs_.size());
    return filter_.outputState(output_channel);
}

const CircularBufferReader& Block::reader(std::size_t input_channel) const
{
    CHECK_LT(input_channel, readers_.size());
    return readers_[input_channel];
}

CircularBufferReader& Block::reader(std::size_t input_channel)
{
    CHECK_LT(input_channel, readers_.size());
    return readers_[input_channel];
}

const CircularBufferWriter& Block::writer(std::size_t output_channel) const
{
    CHECK_LT(output_channel, writers_.size());
    return writers_[output_channel];
}

CircularBufferWriter& Block::writer(std::size_t output_channel)
{
    CHECK_LT(output_channel, writers_.size());
    return writers_[output_channel];
}

const IFilter& Block::filter() const
{
    return filter_;
}

std::string Block::internalState() const
{
    std::stringstream os;
    os << "Block " << typeid(filter_).name() << ", state " << int(state_.load()) << ":" << std::endl;
    for (std::size_t i = 0; i < writers_.size(); ++i)
    {
        os << "Output " << i << ": " << writers_[i].internalState();
    }

    return os.str();
}

void Block::reset()
{
    filter_.reset();

    for (std::size_t i = 0; i < readers_.size(); ++i)
    {
        readers_[i].reset();
    }

    for (std::size_t i = 0; i < writers_.size(); ++i)
    {
        writers_[i].reset();
    }

    state_.store(Block::State::Idle);
}

void Block::process()
{
    for (std::size_t i = 0; i < readers_.size(); ++i)
    {
        inputs_[i] = readers_[i].slice();
    }

    for (std::size_t i = 0; i < writers_.size(); ++i)
    {
        outputs_[i] = writers_[i].slice();
    }

    filter_.process(inputs_.size() ? &inputs_[0] : nullptr, outputs_.size() ? &outputs_[0] : nullptr);

    for (std::size_t i = 0; i < inputs_.size(); ++i)
    {
        std::size_t advance_size = inputs_[i].advancedSize();
        CHECK(
            filter_.inputState(i).mayConsumeNothing() ||
            (advance_size && !(advance_size % filter_.inputState(i).requiredSize()))
        );
        if (advance_size)
        {
            readers_[i].advance(advance_size);
        }
    }

    for (std::size_t i = 0; i < outputs_.size(); ++i)
    {
        std::size_t advance_size = outputs_[i].advancedSize();
        CHECK(advance_size >= filter_.outputState(i).providedSize() || filter_.outputState(i).eof());
        if (advance_size)
        {
            writers_[i].advance(advance_size);
        }
    }
}
