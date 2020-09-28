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

#include <hvylya/filters/ifilter.h>
#include <hvylya/pipelines/async/circular_buffer.h>

namespace hvylya {
namespace pipelines {
namespace async {

class Block: core::NonCopyable
{
  public:
    enum class State: std::int8_t
    {
        Idle,
        Scheduling,
        Scheduled,
        Running
    };

    Block(Pipeline& pipeline, filters::IFilter& filter);

    Block(Block&& block);

    std::atomic<State>& state();

    std::size_t inputChannelsCount() const;

    std::size_t outputChannelsCount() const;

    const filters::InputState& inputState(std::size_t input_channel) const;

    const filters::OutputState& outputState(std::size_t output_channel) const;

    const CircularBufferReader& reader(std::size_t input_channel) const;

    CircularBufferReader& reader(std::size_t input_channel);

    const CircularBufferWriter& writer(std::size_t output_channel) const;

    CircularBufferWriter& writer(std::size_t output_channel);

    const filters::IFilter& filter() const;

    std::string internalState() const;

    void reset();

    void process();

  private:
    Pipeline& pipeline_;
    filters::IFilter& filter_;
    std::atomic<State> state_;
    std::vector<CircularBufferReader> readers_;
    std::vector<CircularBufferWriter> writers_;
    std::vector<core::UntypedSlice> inputs_, outputs_;
};

} // namespace async
} // namespace pipelines
} // namespace hvylya
