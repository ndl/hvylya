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

#include <hvylya/filters/fir_filter.h>

using namespace hvylya::filters;
using namespace hvylya::filters::tests;

template <typename SampleType, typename TapType, std::size_t TapsCount>
class FirFilterTest: public FirFilterTestGeneric<FirFilter<SampleType, TapType, TapsCount>, false>
{
};

typedef FirFilterTest<float, float, 1> FirFilterTestFloat1;
typedef FirFilterTest<float, float, 2> FirFilterTestFloat2;
typedef FirFilterTest<float, float, 3> FirFilterTestFloat3;

typedef FirFilterTest<float, std::complex<float>, 3> FirFilterTestComplexTapsFloat3;
typedef FirFilterTest<std::complex<float>, std::complex<float>, 4> FirFilterTestComplexSamplesFloat4;
typedef FirFilterTest<std::complex<float>, std::complex<float>, 5> FirFilterTestComplexBothFloat5;

TEST_F(FirFilterTestFloat2, SimpleHistory)
{
    EXPECT_TRUE(callFilter({ 1.0f, 2.0f }, { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f }));
}

TEST_F(FirFilterTestFloat2, Accumulating)
{
    createFilter({ 1.0f, 2.0f });
    EXPECT_TRUE(callFilter({ 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f }));
    EXPECT_TRUE(callFilter({ 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f }));
}

TEST_F(FirFilterTestFloat3, OddTapsCount)
{
    createFilter({ 1.0f, 2.0f, 3.0f });
    EXPECT_TRUE(callFilter({ 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f }));
    EXPECT_TRUE(callFilter({ 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f }));
}

TEST_F(FirFilterTestFloat3, LongAndShortSequences)
{
    createFilter({ 1.0f, 2.0f, 3.0f });
    EXPECT_TRUE(callFilter({ 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f }));
    EXPECT_TRUE(callFilter({ 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f }));
}

TEST_F(FirFilterTestComplexTapsFloat3, OddTapsCount)
{
    createFilter({ { 1.0f, 2.0f }, { 3.0f, 4.0f }, { 5.0f, 6.0f} });
    EXPECT_TRUE(callFilter({ 1.0f, 2.0f, 3.0f, 4.0f }));
    EXPECT_TRUE(callFilter({ 5.0f, 6.0f, 7.0f, 8.0f }));
}

TEST_F(FirFilterTestComplexSamplesFloat4, ComplexSamples)
{
    createFilter({ 1.0f, 2.0f, 3.0f, 4.0f });
    EXPECT_TRUE(callFilter({ { 1.0f, 2.0f }, { 3.0f, 4.0f }, { 5.0f, 6.0f }, { 7.0f, 8.0f } }));
    EXPECT_TRUE(callFilter({ { 9.0f, 10.0f }, { 11.0f, 12.0f }, { 13.0f, 14.0f }, { 15.0f, 16.0f } }));
}

TEST_F(FirFilterTestComplexBothFloat5, ComplexBoth)
{
    createFilter({ { 1.0f, 2.0f }, { 3.0f, 4.0f }, { 5.0f, 6.0f },{  7.0f, 8.0f}, { 9.0f, 10.0f } });
    EXPECT_TRUE(callFilter({ { 1.0f, 2.0f }, { 3.0f, 4.0f }, { 5.0f, 6.0f }, { 7.0f, 8.0f } }));
    EXPECT_TRUE(callFilter({ { 9.0f, 10.0f }, { 11.0f, 12.0f }, { 13.0f, 14.0f }, { 15.0f, 16.0f } }));
}

typedef FirFilterRegressionTestGeneric<
    FirFilter<
        float,
        float,
        FirFilterRegressionTapsCount
    >
> FirFilterRegressionTestFloat;

typedef FirFilterRegressionTestGeneric<
    FirFilter<
        std::complex<float>,
        float,
        FirFilterRegressionTapsCount
    >
> FirFilterRegressionTestComplexFloat;

TEST_F(FirFilterRegressionTestFloat, FmBasebandFloat)
{
    run(1);
}

TEST_F(FirFilterRegressionTestComplexFloat, FmBasebandComplexFloat)
{
    run(1);
}
