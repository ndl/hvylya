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

#include <hvylya/filters/filter_type_traits.h>

namespace hvylya {
namespace filters {

template <typename SampleType, typename TapType, std::size_t ForwardTapsCount, std::size_t BackwardTapsCount>
class IirFilter
{
  public:
    typedef FilterTypeMapper<SampleType, TapType> FilterTypes;
    typedef typename FilterTypes::ResultType ResultType;
    enum: std::size_t { MaxTapsCount = ForwardTapsCount > BackwardTapsCount ? ForwardTapsCount : BackwardTapsCount };

    IirFilter(TapType b[ForwardTapsCount], TapType a[BackwardTapsCount]);

    ResultType filter(SampleType input);

  private:
    TapType b_[MaxTapsCount], a_[MaxTapsCount];
    ResultType delayed_[MaxTapsCount];
};

template <typename SampleType, typename TapType, std::size_t ForwardTapsCount, std::size_t BackwardTapsCount>
IirFilter<SampleType, TapType, ForwardTapsCount, BackwardTapsCount>::IirFilter(TapType b[ForwardTapsCount], TapType a[BackwardTapsCount])
{
    std::fill(&b_[0], &b_[MaxTapsCount], TapType(0));
    std::fill(&a_[0], &a_[MaxTapsCount], TapType(0));
    std::fill(&delayed_[0], &delayed_[MaxTapsCount], SampleType(0));

    for (std::size_t i = 0; i < ForwardTapsCount; ++i)
    {
        b_[i] = b[i] / a[0];
    }

    for (std::size_t i = 0; i < BackwardTapsCount - 1; ++i)
    {
        a_[i] = a[i + 1] / a[0];
    }
}

template <typename SampleType, typename TapType, std::size_t ForwardTapsCount, std::size_t BackwardTapsCount>
typename IirFilter<SampleType, TapType, ForwardTapsCount, BackwardTapsCount>::ResultType
IirFilter<SampleType, TapType, ForwardTapsCount, BackwardTapsCount>::filter(SampleType input)
{
    ResultType result(b_[0] * input + delayed_[0]);
    for (std::size_t i = 0; i < MaxTapsCount - 1; ++i)
    {
        delayed_[i] = b_[i + 1] * input - a_[i] * result + delayed_[i + 1];
    }

    return result;
}

} // namespace filters
} // namespace hvylya
