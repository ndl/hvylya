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

#include <hvylya/filters/fir_filter.h>
#include <hvylya/filters/rotator.h>

namespace hvylya {
namespace filters {

template <typename SampleType, typename TapType, class Kernel, std::size_t TapsCount>
class FirTranslatingFilterBase:
    public FirFilterBase<
        SampleType,
        std::complex<typename FilterTypeMapper<SampleType, TapType>::ScalarType>,
        Kernel,
        TapsCount
    >
{
  public:
    typedef FirFilterBase<
        SampleType,
        std::complex<typename FilterTypeMapper<SampleType, TapType>::ScalarType>,
        Kernel,
        TapsCount
    > Base;

    typedef typename FilterTypeMapper<SampleType, TapType>::ScalarType ScalarType;
    typedef std::complex<ScalarType> ResultType;

    FirTranslatingFilterBase(const TapType taps[TapsCount], ScalarType center_frequency, bool compensate_delay = false, std::size_t decimation_rate = 1):
        Base(compensate_delay, decimation_rate),
        rotator_(center_frequency, decimation_rate)
    {
        std::complex<ScalarType> new_taps[TapsCount];
        rotator_.createTaps(new_taps, taps, TapsCount);
        Base::kernel_.setTaps(new_taps);
    }

    virtual void postProcess(ResultType* output, std::size_t output_size) override
    {
        rotator_.rotate(output, output_size);
    }

  protected:
    Rotator<ScalarType> rotator_;
};

template <typename SampleType, typename TapType, std::size_t TapsCount>
class FirTranslatingFilter:
    public FirTranslatingFilterBase<
        SampleType,
        TapType,
        VectorizedKernel<SampleType, std::complex<typename FilterTypeMapper<SampleType, TapType>::ScalarType>, TapsCount>,
        TapsCount
    >
{
  public:
    using FirTranslatingFilterBase<
        SampleType,
        TapType,
        VectorizedKernel<SampleType, std::complex<typename FilterTypeMapper<SampleType, TapType>::ScalarType>, TapsCount>,
        TapsCount
    >::FirTranslatingFilterBase;
};

} // namespace filters
} // namespace hvylya
