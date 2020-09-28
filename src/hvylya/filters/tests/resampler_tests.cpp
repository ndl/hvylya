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

#include <hvylya/filters/tests/test_utils.h>
#include <hvylya/filters/tests/resampler_test_utils.h>

#include <hvylya/filters/resampler.h>

using namespace hvylya::core;
using namespace hvylya::filters;
using namespace hvylya::filters::tests;

template <typename SampleType, typename TapType, std::size_t InterpolationRate, std::size_t DecimationRate, std::size_t TapsCount>
class ResamplerTest: public testing::Test
{
  protected:
    void createFilter(const std::vector<TapType>& taps)
    {
        filter_ = std::make_unique<Filter>(&taps[0]);
    }

    testing::AssertionResult callFilter(const std::vector<SampleType>& input, const std::vector<SampleType>& expected_output)
    {
        AlignedVector<SampleType> input_vector(input.size() + Filter::Padding);
        AlignedVector<SampleType> output_vector((input.size() - TapsCount / InterpolationRate + 1) * InterpolationRate / DecimationRate);

        std::copy(input.begin(), input.end(), input_vector.begin());
        // Fill padding.
        std::fill(input_vector.begin() + input.size(), input_vector.end(), 0);

        Slice<SampleType> input_data(&input_vector[0], input.size());
        Slice<SampleType> output_data(output_vector);

        typename Filter::Inputs inputs = std::make_tuple(std::cref(input_data));
        typename Filter::Outputs outputs = std::make_tuple(std::ref(output_data));

        filter_->process(inputs, outputs);

        return IsOutputExpected(expected_output, output_data, 0.01f);
    }

    testing::AssertionResult callFilter(const std::vector<TapType>& taps, const std::vector<SampleType>& input, const std::vector<SampleType>& expected_output)
    {
        createFilter(taps);
        return callFilter(input, expected_output);
    }

  private:
    typedef Resampler<SampleType, TapType, InterpolationRate, DecimationRate, TapsCount> Filter;
    std::unique_ptr<Filter> filter_;
};

typedef ResamplerTest<float, float, 1, 1, 1> ResamplerTestFloat1;
typedef ResamplerTest<float, float, 1, 1, 2> ResamplerTestFloat2;
typedef ResamplerTest<float, float, 2, 2, 2> ResamplerTestFloat2a;
typedef ResamplerTest<float, float, 2, 3, 4> ResamplerTestFloat4;

TEST_F(ResamplerTestFloat1, OneTap)
{
    EXPECT_TRUE(callFilter({ 2.0f }, { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f }, { 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f }));
}

TEST_F(ResamplerTestFloat2, OneToOne)
{
    EXPECT_TRUE(callFilter({ 2.0f, 1.0f }, { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f }, { 2.0f, 5.0f, 8.0f, 11.0f, 14.0f, 17.0f, 20.0f, 23.0f }));
}

TEST_F(ResamplerTestFloat2a, Accumulating2)
{
    createFilter({ 2.0f, 1.0f });
    // Due to zero stuffing only second TAP coefficient will ever be active.
    EXPECT_TRUE(callFilter({ 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f }, { 4.0f, 8.0f, 12.0f, 16.0f, 20.0f, 24.0f, 28.0f, 32.0f }));
}

TEST_F(ResamplerTestFloat4, TwoToThree)
{
    createFilter({ 3.0f, 2.0f, 1.0f, 0.0f });
    EXPECT_TRUE(
        callFilter(
            { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f, 24.0f },
            { 6.0f, 8.0f, 30.0f, 20.0f, 54.0f, 32.0f, 78.0f, 44.0f, 102.0f, 56.0f, 126.0f, 68.0f, 150.0f, 80.0f, 174.0f, 92.0f }));
}

typedef ResamplerRegressionTestGeneric<
    Resampler<
        float,
        float,
        24,
        25,
        ResamplerRegressionTapsCount
    >
> ResamplerRegressionTestFloat;

typedef ResamplerRegressionTestGeneric<
    Resampler<
        std::complex<float>,
        float,
        24,
        25,
        ResamplerRegressionTapsCount
    >
> ResamplerRegressionTestComplexFloat;

TEST_F(ResamplerRegressionTestFloat, FmAudioResamplerFloat)
{
    run(1);
}

TEST_F(ResamplerRegressionTestComplexFloat, FmAudioResamplerComplexFloat)
{
    run(1);
}
