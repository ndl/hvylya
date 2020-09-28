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
#include <hvylya/filters/ifilter.h>

namespace hvylya {
namespace filters {

template <typename Filter, std::size_t Index = 0>
struct ChannelGeneric
{
    typedef typename Filter::InputChannels InputChannels;
    typedef typename Filter::OutputChannels OutputChannels;

    Filter& filter;

    ChannelGeneric(Filter& f):
        filter(f)
    {
    }
};

template <size_t Index = 0, typename Filter>
ChannelGeneric<Filter, Index> makeChannel(Filter& filter)
{
    return ChannelGeneric<Filter, Index>(filter);
}

template <typename Filter0, std::size_t Index0, typename Filter1, std::size_t Index1>
void connect(ChannelGeneric<Filter0, Index0> channel0, ChannelGeneric<Filter1, Index1> channel1)
{
    connectImpl(channel0, channel1);
}

template <typename Filter0, typename Filter1, std::size_t Index1, typename... TailChannels>
void connect(Filter0& filter0, ChannelGeneric<Filter1, Index1> channel1, TailChannels&... tail_channels)
{
    connectImpl(makeChannel(filter0), channel1);
    connectImplTail(channel1, tail_channels...);
}

template <typename Filter0, std::size_t Index0, typename Filter1, typename... TailChannels>
void connect(ChannelGeneric<Filter0, Index0> channel0, Filter1& filter1, TailChannels&... tail_channels)
{
    connectImpl(channel0, makeChannel(filter1));
    connectImplTail(makeChannel(filter1), tail_channels...);
}

template <typename Filter0, typename Filter1, typename... TailChannels>
void connect(Filter0& filter0, Filter1& filter1, TailChannels&... tail_channels)
{
    connectImpl(makeChannel(filter0), makeChannel(filter1));
    connectImplTail(makeChannel(filter1), tail_channels...);
}

template <typename Filter0, std::size_t Index0, typename Filter1, std::size_t Index1>
void connectImpl(ChannelGeneric<Filter0, Index0> channel0, ChannelGeneric<Filter1, Index1> channel1)
{
    typedef typename core::TypeAt<typename ChannelGeneric<Filter0, Index0>::OutputChannels, Index0>::Type OutputType;
    typedef typename core::TypeAt<typename ChannelGeneric<Filter1, Index1>::InputChannels, Index1>::Type InputType;

    static_assert(
        std::is_same<OutputType, InputType>::value,
        "output type of the parent filter at specified output index doesn't match "
        "input type of the child filter at specified input index"
    );

    Channel input_channel = channel1.filter.inputChannel(Index1);
    Channel output_channel = channel0.filter.outputChannel(Index0);

    std::get<0>(output_channel).addSink(input_channel, std::get<1>(output_channel));
    std::get<0>(input_channel).addSource(output_channel, std::get<1>(input_channel));
}

// There's still smth left - call normal 'connect' function.
template <typename ChannelParam, typename... TailChannels>
void connectImplTail(ChannelParam channel, TailChannels&... tail_channels)
{
    connect(channel, tail_channels...);
}

// Tail call - do nothing.
template <typename ChannelParam>
void connectImplTail(ChannelParam /* channel */) { }

} // namespace filters
} // namespace hvylya
