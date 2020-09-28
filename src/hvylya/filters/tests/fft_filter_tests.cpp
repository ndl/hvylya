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

#include <hvylya/filters/tests/fir_test_utils.h>
#include <hvylya/filters/tests/fft_test_utils.h>

#include <hvylya/filters/fft_filter.h>

using namespace hvylya::core;
using namespace hvylya::filters;
using namespace hvylya::filters::tests;

namespace {

template <typename SampleType, typename TapType>
class FftFilterTest: public FirFilterTestGeneric<FftFilter<SampleType, TapType>, true>
{
};

typedef FftFilterTest<float, float> FftFilterTestFloat;
typedef FftFilterTest<std::complex<float>, float> FftFilterTestComplexFloatSamples;
typedef FftFilterTest<float, std::complex<float>> FftFilterTestComplexFloatTaps;
typedef FftFilterTest<std::complex<float>, std::complex<float>> FftFilterTestComplexFloatBoth;

// Process two blocks to check that block transitions are handled properly.
const std::size_t InputSize = 2 * (512 - roundUp<std::size_t>(3, MaxSimdByteSize / sizeof(float)));

// When both samples and taps are complex numbers, alignment is smaller.
const std::size_t InputSizeComplex = 2 * (512 - roundUp<std::size_t>(3, MaxSimdByteSize / sizeof(std::complex<float>)));

} // anonymous namespace

TEST_F(FftFilterTestFloat, SimpleHistory)
{
    std::vector<float> input;
    for (std::size_t i = 0; i < InputSize; ++i)
    {
        input.push_back(i + 1);
    }

    EXPECT_TRUE(callFilter({ 1.0f, 2.0f, 3.0f, 4.0f }, input, 4));
}

TEST_F(FftFilterTestComplexFloatSamples, SimpleHistory)
{
    std::vector<std::complex<float>> input;
    for (std::size_t i = 0; i < InputSize; ++i)
    {
        input.push_back(std::complex<float>(i + 1, i + 1));
    }

    EXPECT_TRUE(callFilter({ 1.0f, 2.0f, 3.0f, 4.0f }, input, 4));
}

TEST_F(FftFilterTestComplexFloatTaps, SimpleHistory)
{
    std::vector<float> input;
    for (std::size_t i = 0; i < InputSize; ++i)
    {
        input.push_back(i + 1);
    }

    EXPECT_TRUE(callFilter({ { 1.0f, 0.0f }, { 2.0f, 0.0f }, { 3.0f, 0.0f }, { 4.0f, 0.0f } }, input, 4));
}

TEST_F(FftFilterTestComplexFloatBoth, SimpleHistory)
{
    std::vector<std::complex<float>> input;
    for (std::size_t i = 0; i < InputSizeComplex; ++i)
    {
        input.push_back(std::complex<float>(i + 1, i + 1));
    }

    EXPECT_TRUE(callFilter({ { 1.0f, 0.0f }, { 2.0f, 0.0f }, { 3.0f, 0.0f }, { 4.0f, 0.0f } }, input, 4));
}

typedef FirFilterRegressionTestGeneric<FftFilter<float, float>> FftFilterRegressionTestFloat;
typedef FirFilterRegressionTestGeneric<FftFilter<std::complex<float>, float>> FftFilterRegressionTestComplexFloat;

TEST_F(FftFilterRegressionTestFloat, FmBasebandFloat)
{
    run(1, FirFilterRegressionTapsCount);
}

TEST_F(FftFilterRegressionTestComplexFloat, FmBasebandComplexFloat)
{
    run(1, FirFilterRegressionTapsCount);
}
