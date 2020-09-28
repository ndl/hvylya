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

#include <hvylya/filters/fft_filter.h>
#include <hvylya/filters/fftw_convolver.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename SampleType, typename TapType>
FftFilter<SampleType, TapType>::FftFilter(
    const TapType taps[],
    const std::size_t taps_count,
    bool compensate_delay,
    std::size_t decimation_rate
):
    FftFilter(taps_count, compensate_delay, decimation_rate)
{
    // Alignment is not so much a problem here, but we need to pad taps with zeros anyway,
    // plus conversion to ResultType is required.
    AlignedVector<ResultType> tmp_taps(block_size_);
    std::copy(&taps[0], &taps[taps_count], &tmp_taps[0]);

    fft_algorithm_ =
        std::make_unique<FftwConvolver<SampleType, ResultType>>(
            &tmp_taps[0],
            block_size_,
            &history_[0],
            &transformed_samples_[0]
        );
}

template <typename SampleType, typename TapType>
void FftFilter<SampleType, TapType>::reset()
{
    Base::reset();
    remaining_skip_ = 0;
}

template <typename SampleType, typename TapType>
void FftFilter<SampleType, TapType>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    const auto& input_data = std::get<0>(input);
    auto& output_data = std::get<0>(output);

    // Require aligned data, as in this case we can save one data copying operation.
    CHECK(isPointerAligned(&input_data[0]));

    std::size_t input_data_size, output_data_size;

    // We can handle things in more efficient way if we're not doing decimation.
    if (decimation_rate_ == 1)
    {
        output_data_size =
            std::min(
                roundDown(input_data.size() - block_shift_, output_block_size_),
                roundDown(output_data.size(), output_block_size_)
            );
        input_data_size = output_data_size + block_shift_;

        CHECK_GE(input_data_size, block_size_);
        CHECK_GE(output_data_size, output_block_size_);

        // Iterate in reverse direction, so that all but the last iteration can write directly to the output buffer.
        // Initial block_shift_ elements on each iteration are garbage, but they will be overwritten by the
        // subsequent iteration.
        //
        // Hack: use overflown value of input_index to wrap it back and check for zero.
        for (
            std::size_t input_index = input_data_size - block_size_, output_index = output_data_size - output_block_size_;
            input_index + output_block_size_ != 0;
            input_index -= output_block_size_, output_index -= output_block_size_
        )
        {
            const SampleType* input_ptr = getInputPointer(&input_data[input_index]);
            ResultType* output_ptr = getOutputPointer(&output_data[0], output_index);

            fft_algorithm_->convolve(input_ptr, output_ptr);

            // Either the very last iteration or non-aligned data.
            if (output_ptr == &transformed_samples_[0])
            {
                std::copy(&transformed_samples_[block_shift_], &transformed_samples_[block_size_], &output_data[output_index]);
            }
        }
    }
    else
    {
        input_data_size =
            std::min(
                roundDown(input_data.size() - block_shift_, output_block_size_),
                roundDown(output_data.size() * decimation_rate_ + remaining_skip_ + decimation_rate_ - 1, output_block_size_)
            ) + block_shift_;

        CHECK_GE(input_data_size, block_size_);

        std::size_t output_index = 0;
        for (std::size_t input_index = 0; input_index + block_size_ <= input_data_size; input_index += output_block_size_)
        {
            const SampleType* input_ptr = getInputPointer(&input_data[input_index]);

            fft_algorithm_->convolve(input_ptr, &transformed_samples_[0]);

            std::size_t index = block_shift_ + remaining_skip_;
            for (; index < block_size_; index += decimation_rate_, ++output_index)
            {
                output_data[output_index] = transformed_samples_[index];
            }

            remaining_skip_ = index - block_size_;
        }

        output_data_size = output_index;
        CHECK_LE(output_data_size, output_data.size());
    }

    postProcess(&output_data[0], output_data_size);

    input_data.advance(input_data_size - block_shift_);
    output_data.advance(output_data_size);
}

template <typename SampleType, typename TapType>
FftFilter<SampleType, TapType>::FftFilter(
    const std::size_t taps_count,
    bool compensate_delay,
    std::size_t decimation_rate
):
    decimation_rate_(decimation_rate),
    remaining_skip_(0),
    compensate_delay_(compensate_delay)
{
    CHECK_NE(taps_count, 0) << "Expected at least one tap";
    CHECK_NE(decimation_rate, 0) << "Decimation rate cannot be zero";
    CHECK(!compensate_delay_ || taps_count % 2) << "Taps count must be odd for delay compensation";
    CHECK(!compensate_delay_ || !(((taps_count - 1) / 2) % SampleVector::Elements)) <<
        "(taps count - 1) / 2 must be a multiple of the alignment for delay compensation";

    block_size_ = std::max<std::size_t>(MinBlockSize, roundUpToPowerOfTwo(BlockRatio * (taps_count - 1)));
    block_shift_ = roundUp<std::size_t>(taps_count - 1, BlockShiftAlignmentInElements);
    output_block_size_ = block_size_ - block_shift_;

    history_.resize(block_size_);
    transformed_samples_.resize(block_size_);

    Base::inputState(0).setHistorySize(block_shift_);
    Base::inputState(0).setRequiredSize(output_block_size_);
    Base::outputState(0).setRequiredSize((output_block_size_ + (decimation_rate_ - 1)) / decimation_rate_);

    if (compensate_delay_)
    {
        Base::inputState(0).setDelay((taps_count - 1) / 2);
    }
}

// Do nothing in base version.
template <typename SampleType, typename TapType>
void FftFilter<SampleType, TapType>::postProcess(ResultType* /* output */, std::size_t /* output_size */)
{
}

template <typename SampleType, typename TapType>
const SampleType*
FftFilter<SampleType, TapType>::getInputPointer(const SampleType* input)
{
    if (!isPointerAligned(input))
    {
        std::copy(input, input + block_size_, &history_[0]);
        return &history_[0];
    }
    else
    {
        return input;
    }
}

template <typename SampleType, typename TapType>
typename FftFilter<SampleType, TapType>::ResultType*
FftFilter<SampleType, TapType>::getOutputPointer(ResultType* output, std::size_t index)
{
    if (!isPointerAligned(output) || index < block_shift_)
    {
        return &transformed_samples_[0];
    }
    else
    {
        return output + index - block_shift_;
    }
}

template class hvylya::filters::FftFilter<float, float>;
template class hvylya::filters::FftFilter<float, std::complex<float>>;
template class hvylya::filters::FftFilter<std::complex<float>, float>;
template class hvylya::filters::FftFilter<std::complex<float>, std::complex<float>>;
