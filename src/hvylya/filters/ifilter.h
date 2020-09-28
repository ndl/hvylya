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

#include <hvylya/core/common.h>
#include <hvylya/core/untyped_slice.h>

namespace hvylya {
namespace filters {

class InputState;
class OutputState;
class IFilter;

typedef std::tuple<IFilter&, std::size_t> Channel;

template <typename Filter, std::size_t Index>
struct ChannelGeneric;

class IFilter: core::NonCopyable
{
  public:
    virtual ~IFilter() { }

    virtual std::size_t inputChannelsCount() const = 0;

    virtual std::size_t outputChannelsCount() const = 0;

    virtual const InputState& inputState(std::size_t input_channel) const = 0;

    virtual const OutputState& outputState(std::size_t output_channel) const = 0;

    virtual const std::vector<Channel>& sources(std::size_t input_channel) const = 0;

    virtual const std::vector<Channel>& sinks(std::size_t output_channel) const = 0;

    virtual void reset() = 0;

    virtual void process(core::UntypedSlice inputs[], core::UntypedSlice outputs[]) = 0;

  protected:
    virtual void addSource(Channel source_output_channel, std::size_t input_channel) = 0;

    virtual void addSink(Channel sink_input_channel, std::size_t output_channel) = 0;

  private:
    // Needed for addSource / addSink.
    template <typename Filter0, std::size_t Index0, typename Filter1, std::size_t Index1>
    friend void connectImpl(ChannelGeneric<Filter0, Index0> channel0, ChannelGeneric<Filter1, Index1> channel1);
};

} // namespace filters
} // namespace hvylya
