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

#include <hvylya/filters/filter_generic.h>

namespace hvylya {
namespace filters {

using namespace hvylya::core;

template <typename T, std::size_t ChannelsCount = 1>
class NullSink:
    public FilterGeneric<
        typename TypeDuplicator<T, ChannelsCount>::Type,
        TypeList<>
    >
{
  public:
    static_assert(ChannelsCount > 0, "NullSink requires at least one channel");

    typedef typename FilterBaseType<NullSink>::Type Base;

    NullSink():
        samples_(0)
    {
    }

    std::size_t samples() const
    {
        return samples_;
    }

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& /* output */) override
    {
        std::size_t input_data_size = std::numeric_limits<std::size_t>::max();
        forEachTupleElement(
            input,
            [&input_data_size](const auto& channel, auto /* channel_index */)
            {
                input_data_size = std::min(input_data_size, channel.size());
            }
        );

        forEachTupleElement(
            input,
            [=](const auto& channel, auto /* channel_index */)
            {
                channel.advance(input_data_size);
            }
        );

        samples_ += input_data_size;
    }

  private:
    std::size_t samples_;
};

} // namespace filters
} // namespace hvylya
