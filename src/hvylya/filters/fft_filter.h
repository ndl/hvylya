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

#include <hvylya/core/aligned_vector.h>
#include <hvylya/core/simd_vector.h>

#include <hvylya/filters/filter_generic.h>
#include <hvylya/filters/filter_type_traits.h>
#include <hvylya/filters/fftw_convolver.h>

namespace hvylya {
namespace filters {

template <typename SampleType, typename TapType>
class FftFilter:
    public FilterGeneric<
        core::TypeList<SampleType>,
        core::TypeList<typename FilterTypeMapper<SampleType, TapType>::ResultType>
    >
{
  public:
    typedef typename FilterBaseType<FftFilter>::Type Base;
    typedef FilterTypeMapper<SampleType, TapType> FirTypes;
    typedef typename FirTypes::ResultType ResultType;

    enum: std::size_t
    {
        // FFT FIR filter operation doesn't need padding.
        Padding = 0
    };

    FftFilter(const TapType taps[], const std::size_t taps_count, bool compensate_delay = false, std::size_t decimation_rate = 1);

    virtual void reset() override;

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override;

  protected:
    typedef core::SimdVector<SampleType, core::Aligned> SampleVector;
    typedef core::SimdVector<TapType, core::Aligned> TapVector;

    enum: std::size_t
    {
        MinBlockSize = 512,
        BlockRatio = 4,
        BlockShiftAlignmentInElements = TapVector::Elements > SampleVector::Elements ? TapVector::Elements : SampleVector::Elements
    };

    // Alignment is needed for vectorized version.
    core::AlignedVector<SampleType> history_;
    core::AlignedVector<ResultType> transformed_samples_;
    std::unique_ptr<FftwConvolver<SampleType, ResultType>> fft_algorithm_;
    std::size_t block_size_, block_shift_, output_block_size_, decimation_rate_, remaining_skip_;
    bool compensate_delay_;

    FftFilter(const std::size_t taps_count, bool compensate_delay, std::size_t decimation_rate);

    virtual void postProcess(ResultType* output, std::size_t output_size);

    const SampleType* getInputPointer(const SampleType* input);

    ResultType* getOutputPointer(ResultType* output, std::size_t index);
};

} // namespace filters
} // namespace hvylya
