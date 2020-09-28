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

#include <hvylya/core/simd_vector.h>
#include <hvylya/filters/filter_generic.h>
#include <hvylya/filters/filter_type_traits.h>

namespace hvylya {
namespace filters {

template <typename SampleType, typename TapType, class Kernel, std::size_t TapsCount>
class FirFilterBase:
    public FilterGeneric<
        core::TypeList<SampleType>,
        core::TypeList<typename FilterTypeMapper<SampleType, TapType>::ResultType>
    >
{
  public:
    typedef typename FilterBaseType<FirFilterBase>::Type Base;
    typedef FilterTypeMapper<SampleType, TapType> FirTypes;
    typedef typename FirTypes::ResultType ResultType;

    enum: std::size_t
    {
        Padding = Kernel::Padding
    };

    FirFilterBase(const TapType taps[TapsCount], bool compensate_delay = false, std::size_t decimation_rate = 1):
        FirFilterBase(compensate_delay, decimation_rate)
    {
        kernel_.setTaps(taps);
    }

    static ResultType filter(const Kernel& kernel, core::Slice<SampleType> input_data, std::size_t input_index)
    {
        const SampleType* input_ptr = &input_data[0];
        std::size_t input_offset = 0;
        core::alignPointer(input_ptr, input_offset);
        return kernel.apply(input_ptr, input_index + input_offset);
    }

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override
    {
        const auto& input_data = std::get<0>(input);
        auto& output_data = std::get<0>(output);

        const std::size_t output_data_size =
            std::min(
                output_data.size(),
                (input_data.size() - TapsCount + 1) / decimation_rate_
            );
        const std::size_t input_data_size = output_data_size * decimation_rate_;

        const SampleType* input_ptr = &input_data[0];
        std::size_t input_offset = 0;
        core::alignPointer(input_ptr, input_offset);

        // This loop is pretty tight, moving 'input_offset' addition to loop bounds instead of performing it
        // every cycle results in better performance.
        const std::size_t input_data_size_offsetted = input_data_size + input_offset;
        std::size_t input_index = input_offset, output_index = 0;

        for (; input_index < input_data_size_offsetted; input_index += decimation_rate_, ++output_index)
        {
            output_data[output_index] = kernel_.apply(input_ptr, input_index);
        }

        postProcess(&output_data[0], output_data_size);

        CHECK_EQ(input_index, input_data_size_offsetted);
        CHECK_EQ(output_index, output_data_size);

        input_data.advance(input_data_size);
        output_data.advance(output_data_size);
    }

  protected:
    Kernel kernel_;
    bool compensate_delay_;
    std::size_t decimation_rate_;

    FirFilterBase(bool compensate_delay, std::size_t decimation_rate):
        compensate_delay_(compensate_delay),
        decimation_rate_(decimation_rate)
    {
        static_assert(TapsCount != 0, "Expected at least one tap");
        CHECK_NE(decimation_rate, 0) << "Decimation rate cannot be zero";
        CHECK(!compensate_delay_ || TapsCount % 2) << "Taps count must be odd for delay compensation";
        // This is not strictly required for the current implementation of the FIR filter
        // as it accepts unaligned data thanks to the use of the taps bank, but this
        // can change in the future + FFT filter requires alignment anyway, and migrating between
        // FIR & FFT is easier if taps requirements are the same.
        CHECK(!compensate_delay_ || !(((TapsCount - 1) / 2) % Kernel::SampleVector::Elements)) <<
            "(taps count - 1) / 2 must be a multiple of the alignment for delay compensation";

        Base::inputState(0).setHistorySize(TapsCount - 1);
        // We want to have enough input elements to produce at least one output sample.
        Base::inputState(0).setRequiredSize(decimation_rate);
        Base::inputState(0).setPadding(Padding);

        if (compensate_delay_)
        {
            Base::inputState(0).setDelay((TapsCount - 1) / 2);
        }
    }

    // Do nothing in base version.
    virtual void postProcess(ResultType* /* output */, std::size_t /* output_size */) { }
};

// Note: for the case of 'complex<float>' samples and 'float' taps
// more optimal implementation seems to be to pre-expand taps during
// initialization and then load them directly without any expansion in the apply().
// Also, counterintuitively, FMA seems to be slower in that case than separate
// additions and multiplications - likely due to the way Clang arranges instructions?
//
// However, given it requires duplication of the whole kernel class - for now
// let's not bother with it (although it gives ~20% performance increase, so
// if it's determined to be critical - we can still consider that specialization).
template <typename SampleType, typename TapType, std::size_t TapsCount>
class VectorizedKernel
{
  public:
    typedef typename FilterTypeMapper<SampleType, TapType>::ResultType ResultType;

    typedef core::SimdVector<ResultType, core::Aligned> ResultVector;
    typedef core::SimdVectorImpl<SampleType, ResultVector::Elements * sizeof(SampleType), core::Aligned> SampleVector;
    typedef core::SimdVectorImpl<TapType, ResultVector::Elements * sizeof(TapType), core::Aligned> TapVector;

    enum: std::size_t
    {
        // Derivation is as follows. The last stride for taps starts with (Align - 1) zeroes followed by single tap value
        // (hence +1 in the formula) followed by the number of vectors that is necessary to completely contain (TapsCount - 1)
        // values. The latter is easy to calculate as ((TapsCount - 1) + (Align - 1)) / Align, hence arriving to the final
        // expression.
        AlignedTapsCount = (1 + ((TapsCount - 1) + (SampleVector::Elements - 1)) / SampleVector::Elements) * SampleVector::Elements,
        // FIR filter operation requires 2 * Align padding.
        //
        // Explanation is as follows. When using taps bank, we need as many samples at the input
        // as there are aligned taps (= VectorizedKernel::AlignedTapsCount).
        //
        // Given that we have TapsCount - 1 history samples, the padding has to be >= AlignedTapsCount - (TapsCount - 1).
        //
        // Due to the way AlignedTapsCount was constructed, it's second component is always
        // larger or equal to TapsCount - 1, the proof is easy once TapsCount - 1 is expressed as
        // Align * N + M (1), then it's guaranteed that result is >= Align * N + Align (2),
        // where (2) is bigger than (1).
        //
        // In practice it means that the required padding is always >= Align but < 2 * Align.
        Padding = 2 * SampleVector::Elements
    };

    VectorizedKernel() { }

    VectorizedKernel(const TapType taps[TapsCount])
    {
        setTaps(taps);
    }

    void setTaps(const TapType taps[TapsCount])
    {
        for (std::size_t stride = 0; stride < SampleVector::Elements; ++stride)
        {
            taps_[stride].resize(AlignedTapsCount);
            std::fill(&taps_[stride][0], &taps_[stride][AlignedTapsCount], TapType());

            for (std::size_t index = 0; index < TapsCount; ++index)
            {
                taps_[stride][index + stride] = taps[TapsCount - index - 1];
            }
        }
    }

    ResultType apply(const SampleType* data, std::size_t input_index) const
    {
        std::size_t data_index = input_index & ~(SampleVector::Elements - 1);
        std::size_t stride = input_index & (SampleVector::Elements - 1);

        ResultType init_value = ResultType();
        ResultVector vec_result(init_value);
        for (std::size_t i = 0; i < AlignedTapsCount; i += SampleVector::Elements)
        {
            const SampleVector sample = *reinterpret_cast<const SampleVector*>(&data[data_index + i]);
            const TapVector taps = *reinterpret_cast<const TapVector*>(&taps_[stride][i]);
            vec_result =
                core::fusedMultiplyAdd(
                    core::extendAdjacentAs<SampleExtendedVector>(sample),
                    core::extendAdjacentAs<TapExtendedVector>(taps),
                    vec_result
                );
        }

        return sum(vec_result);
    }

  private:
    typedef core::SimdVector<SampleType, core::Aligned> SampleExtendedVector;
    typedef core::SimdVector<TapType, core::Aligned> TapExtendedVector;

    core::AlignedVector<TapType> taps_[SampleVector::Elements];
};

template <typename SampleType, typename TapType, std::size_t TapsCount>
class FirFilter:
    public FirFilterBase<
        SampleType,
        TapType,
        VectorizedKernel<SampleType, TapType, TapsCount>,
        TapsCount
    >
{
  public:
    using FirFilterBase<
        SampleType,
        TapType,
        VectorizedKernel<SampleType, TapType, TapsCount>,
        TapsCount
    >::FirFilterBase;
};

} // namespace filters
} // namespace hvylya
