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

#include <hvylya/filters/fft_filter.h>
#include <hvylya/filters/rotator.h>

namespace hvylya {
namespace filters {

template <typename SampleType, typename TapType>
class FftTranslatingFilter:
    public FftFilter<
        SampleType,
        std::complex<typename FilterTypeMapper<SampleType, TapType>::ScalarType>
    >
{
  public:
    typedef FftFilter<
        SampleType,
        std::complex<typename FilterTypeMapper<SampleType, TapType>::ScalarType>
    > Base;

    typedef typename FilterTypeMapper<SampleType, TapType>::ScalarType ScalarType;
    typedef std::complex<ScalarType> ResultType;

    FftTranslatingFilter(
        const TapType taps[],
        const std::size_t taps_count,
        ScalarType center_frequency,
        bool compensate_delay = false,
        std::size_t decimation_rate = 1
    );

    virtual void reset() override;

  protected:
    Rotator<ScalarType> rotator_;

    virtual void postProcess(ResultType* output, std::size_t output_size) override;
};

} // namespace filters
} // namespace hvylya
