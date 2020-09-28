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

#include <hvylya/filters/iir_filter.h>

namespace hvylya {
namespace filters {

template <typename SampleType>
class IirFiltersDesigner: core::NonCopyable
{
  public:
    typedef typename hvylya::core::NumericTypeMapper<SampleType>::ScalarType ScalarType;
    typedef ScalarType TapType;
    typedef IirFilter<SampleType, ScalarType, 2, 2> IirFirstOrderFilter;
    typedef IirFilter<SampleType, ScalarType, 3, 3> IirBiquadFilter;

    static IirBiquadFilter createLowpassBiquadFilter(ScalarType corner_freq, ScalarType bandwidth);

    static IirFirstOrderFilter createLowpassFirstOrderFilter(ScalarType corner_freq);
};

} // namespace filters
} // namespace hvylya
