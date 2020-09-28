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

namespace hvylya {
namespace filters {

template <typename SampleType, typename TapType, std::size_t InterpolationRate, std::size_t DecimationRate, std::size_t TapsCount>
class Resampler:
    public FilterGeneric<
        core::TypeList<SampleType>,
        core::TypeList<typename FilterTypeMapper<SampleType, TapType>::ResultType>
    >
{
  public:
    typedef typename FilterBaseType<Resampler>::Type Base;
    typedef FilterTypeMapper<SampleType, TapType> FirTypes;

    enum: std::size_t
    {
        FiltersCount = (TapsCount + InterpolationRate - 1) / InterpolationRate
    };

    typedef VectorizedKernel<SampleType, TapType, FiltersCount> Kernel;
    typedef FirFilter<SampleType, TapType, FiltersCount> Filter;

    enum: std::size_t
    {
        Padding = Kernel::Padding
    };

    Resampler(const TapType taps[TapsCount]):
        kernel_index_(0)
    {
        static_assert(TapsCount != 0, "Expected at least one tap");
        static_assert(InterpolationRate != 0, "Interpolation rate cannot be zero");
        static_assert(DecimationRate != 0, "Decimation rate cannot be zero");

        for (std::size_t kernel_index = 0; kernel_index < InterpolationRate; ++kernel_index)
        {
            TapType new_taps[FiltersCount] = { TapType() };
            for (std::size_t tap_index = kernel_index; tap_index < TapsCount; tap_index += InterpolationRate)
            {
                // Due to zero stuffing we need to scale filter taps to maintain the same gain.
                new_taps[tap_index / InterpolationRate] = InterpolationRate * taps[tap_index];
            }

            kernels_[kernel_index] = std::make_unique<Kernel>(new_taps);
        }

        Base::inputState(0).setHistorySize(FiltersCount - 1);
        // Set the padding required by FIR filter.
        Base::inputState(0).setPadding(Padding);
        // We're decimating, so we cannot guarantee we have any output for the smallest possible input.
        Base::outputState(0).setProvidedSize(0);
    }

    virtual void reset() override
    {
        Base::reset();
        kernel_index_ = 0;
    }

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override
    {
        const auto& input_data = std::get<0>(input);
        auto& output_data = std::get<0>(output);

        const std::size_t input_data_size = input_data.size() - FiltersCount + 1;
        const std::size_t output_data_size = output_data.size();

        std::size_t input_index = 0, current_kernel_index = kernel_index_;

        // Finish adjusting the filter index from the previous data.
        while (current_kernel_index >= InterpolationRate && input_index < input_data_size)
        {
            current_kernel_index -= InterpolationRate;
            ++input_index;
        }

        std::size_t output_index = 0;
        for(; input_index < input_data_size && output_index < output_data_size; ++output_index)
        {
            output_data[output_index] = Filter::filter(*kernels_[current_kernel_index], input_data, input_index);

            current_kernel_index += DecimationRate;
            while (current_kernel_index >= InterpolationRate && input_index < input_data_size)
            {
                current_kernel_index -= InterpolationRate;
                ++input_index;
            }
        }

        kernel_index_ = current_kernel_index;

        input_data.advance(input_index);
        output_data.advance(output_index);
    }

  private:
    std::size_t kernel_index_;
    std::unique_ptr<Kernel> kernels_[InterpolationRate];
};

} // namespace filters
} // namespace hvylya
