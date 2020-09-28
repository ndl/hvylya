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

#include <hvylya/filters/fm/fm_constants.h>

#include <hvylya/filters/tests/filter_test_utils.h>
#include <hvylya/filters/tests/test_utils.h>

namespace hvylya {
namespace filters {
namespace tests {

template <typename Filter, bool Aligned>
class FirFilterTestGeneric: public testing::Test
{
  protected:
    typedef typename Filter::FirTypes FirTypes;
    typedef typename FirTypes::SampleType SampleType;
    typedef typename FirTypes::TapType TapType;
    typedef typename FirTypes::ResultType ResultType;

    template <typename... Args>
    void createFilter(const std::vector<TapType>& taps, Args... args)
    {
        taps_ = taps;
        aligned_taps_count_ = Aligned ?
            core::roundUp<std::size_t>(
                taps.size() - 1,
                std::max<std::size_t>(
                    core::AlignedVector<SampleType>::AlignmentInElements,
                    core::AlignedVector<TapType>::AlignmentInElements
                )
            ) + 1 : taps_.size();
        filter_ = std::make_unique<Filter>(&taps_[0], args...);
        history_.assign(aligned_taps_count_, SampleType());
    }

    testing::AssertionResult callFilter(const std::vector<SampleType>& input)
    {
        core::AlignedVector<SampleType> input_vector(input.size() + aligned_taps_count_ - 1 + Filter::Padding);
        core::AlignedVector<ResultType> output_vector(input.size());

        std::copy(
            history_.begin() + 1,
            history_.begin() + typename std::deque<SampleType>::difference_type(aligned_taps_count_),
            input_vector.begin()
        );
        std::copy(input.begin(), input.end(), input_vector.begin() + aligned_taps_count_ - 1);
        // Fill padding.
        std::fill(input_vector.begin() + input.size() + aligned_taps_count_ - 1, input_vector.end(), 0);

        core::Slice<SampleType> input_data(input_vector);
        core::Slice<ResultType> output_data(output_vector);

        typename Filter::Inputs inputs = std::make_tuple(std::cref(input_data));
        typename Filter::Outputs outputs = std::make_tuple(std::ref(output_data));

        filter_->process(inputs, outputs);

        std::vector<ResultType> expected_output;
        getFilterResult(input, expected_output);

        return IsOutputExpected(expected_output, output_data, 0.01f);
    }

    template <typename... Args>
    testing::AssertionResult callFilter(const std::vector<TapType>& taps, const std::vector<SampleType>& input, Args... args)
    {
        createFilter(taps, args...);
        return callFilter(input);
    }

    void getFilterResult(const std::vector<SampleType>& input, std::vector<ResultType>& output)
    {
        output.resize(input.size());
    
        for (std::size_t i = 0; i < input.size(); ++i)
        {
            history_.pop_front();
            history_.push_back(input[i]);
            ResultType result = ResultType();
            for (std::size_t j = 0; j < taps_.size(); ++j)
            {
                result += history_[j + aligned_taps_count_ - taps_.size()] * taps_[taps_.size() - j - 1];
            }
            output[i] = result;
        }
    }

  private:
    std::unique_ptr<Filter> filter_;
    std::vector<TapType> taps_;
    std::deque<SampleType> history_;
    std::size_t aligned_taps_count_;
};

enum: std::size_t
{
    FirFilterRegressionTapsCount = fm::FmBasebandTapsCount
};

template <typename Filter>
class FirFilterRegressionTestGeneric: public FilterRegressionTestGeneric<Filter>
{
  public:
    FirFilterRegressionTestGeneric()
    {
        std::vector<double> taps(&fm::FmBasebandTaps[0], &fm::FmBasebandTaps[fm::FmBasebandTapsCount]);
        FilterRegressionTestGeneric<Filter>::createTaps(taps);
    }

  protected:
    virtual VerificationInfo verificationInfo() const override
    {
        VerificationInfo info;
        info.taps_count = FirFilterRegressionTapsCount;
        info.passband = fm::FmChannelBandwidth / fm::InputSamplingRate;
        info.stopband = fm::FmChannelStopband / fm::InputSamplingRate;
        info.freq_multiplier = 1.0;
        info.phase_shift = (info.taps_count - 1.0) / 2.0;
        info.accuracy = 0.001;
        return info;
    }
};

} // namespace tests
} // namespace filters
} // namespace hvylya
