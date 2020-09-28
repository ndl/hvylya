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
#include <hvylya/filters/type_utils.h>

namespace hvylya {
namespace filters {

template <typename Callable>
class MapperFilter:
    public FilterGeneric<
        typename CallableArgsExtractor<Callable>::InputTypes,
        typename CallableArgsExtractor<Callable>::OutputTypes
    >
{
  public:
    typedef typename FilterBaseType<MapperFilter>::Type Base;

    MapperFilter(Callable callable):
        callable_(callable)
    {
    }

    MapperFilter(MapperFilter<Callable>&& filter):
        callable_(std::move(filter.callable_))
    {
    }

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override
    {
        std::size_t data_size = std::numeric_limits<std::size_t>::max();

        auto data_size_calculator =
            [&data_size](const auto& channel, auto /* channel_index */)
            {
                data_size = std::min(data_size, channel.size());
            };

        core::forEachTupleElement(input, data_size_calculator);
        core::forEachTupleElement(output, data_size_calculator);

        for (std::size_t i = 0; i < data_size; ++i)
        {
            processImpl<0, 0>(input, output, i);
        }

        auto channel_advancer =
            [data_size](auto& channel, auto /* channel_index */)
            {
                channel.advance(data_size);
            };

        core::forEachTupleElement(input, channel_advancer);
        core::forEachTupleElement(output, channel_advancer);
    }

  private:
    Callable callable_;

    template <
        std::size_t InputIndex,
        std::size_t OutputIndex,
        typename... CallableTypes,
        typename std::enable_if<InputIndex < Base::InputsCount>::type* = nullptr
    >
    void processImpl(
        const typename Base::Inputs& input,
        const typename Base::Outputs& output,
        std::size_t index,
        CallableTypes... args
    )
    {
        processImpl<InputIndex + 1, OutputIndex>(input, output, index, args..., std::cref(std::get<InputIndex>(input)[index]));
    }

    template <
        std::size_t InputIndex,
        std::size_t OutputIndex,
        typename... CallableTypes,
        typename std::enable_if<InputIndex == Base::InputsCount>::type* = nullptr,
        typename std::enable_if<OutputIndex < Base::OutputsCount>::type* = nullptr
    >
    void processImpl(
        const typename Base::Inputs& input,
        const typename Base::Outputs& output,
        std::size_t index,
        CallableTypes... args
    )
    {
        processImpl<InputIndex, OutputIndex + 1>(input, output, index, args..., std::ref(std::get<OutputIndex>(output)[index]));
    }

    template <
        std::size_t InputIndex,
        std::size_t OutputIndex,
        typename... CallableTypes,
        typename std::enable_if<InputIndex == Base::InputsCount>::type* = nullptr,
        typename std::enable_if<OutputIndex == Base::OutputsCount>::type* = nullptr
    >
    void processImpl(
        const typename Base::Inputs& /* input */,
        const typename Base::Outputs& /* output */,
        std::size_t /* index */,
        CallableTypes... args
    )
    {
        callable_(args...);
    }
};

template <typename Callable>
MapperFilter<Callable>&& makeMapperFilter(Callable callable)
{
    return std::move(MapperFilter<Callable>(callable));
}

} // namespace filters
} // namespace hvylya
