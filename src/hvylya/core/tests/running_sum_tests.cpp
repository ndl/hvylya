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

#include <hvylya/core/running_sum.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::core;

TEST(RunningSum, SumFloat)
{
    const std::size_t SumSize = 1024;
    const std::size_t Samples = 10000000;
    RunningSum<float> sum(SumSize);

    for (std::size_t i = Samples; i >= 1; --i)
    {
        sum.add(float(M_PI * i));
    }

    float expected = float(SumSize * (SumSize + 1) / 2 * M_PI);
    float actual = sum.sum();

    EXPECT_TRUE(!sum.empty());
    EXPECT_EQ(SumSize, sum.size());
    EXPECT_NEAR(expected, actual, 50.0f);

    sum.clear();
    EXPECT_TRUE(sum.empty());
    EXPECT_EQ(0, sum.size());
}

TEST(RunningSum, SumDouble)
{
    const std::size_t SumSize = 1024;
    const std::size_t Samples = 10000000;
    RunningSum<double> sum(SumSize);

    for (std::size_t i = Samples; i >= 1; --i)
    {
        sum.add(M_PI * i);
    }

    double expected = SumSize * (SumSize + 1) / 2 * M_PI;
    double actual = sum.sum();

    EXPECT_NEAR(expected, actual, 1e-5f);
}
