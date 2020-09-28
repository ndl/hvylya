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

#include <hvylya/core/lagrange_interpolator.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::core;

namespace {
    double cubicFunc(double x)
    {
        return 2 * x * x * x - 3 * x * x + 4 * x - 5;
    }

    double cubicFunc1(double x)
    {
        return 6 * x * x - 6 * x + 4;
    }

    double cubicFunc2(double x)
    {
        return 12 * x - 6;
    }

    double Error = 1e-11;
}

TEST(LagrangeInterpolator, Quadric)
{
    std::vector<double> args, values;
    // y = x^2
    args.push_back(1); values.push_back(1);
    args.push_back(2); values.push_back(4);
    args.push_back(3); values.push_back(9);

    LagrangeInterpolator<double> interpolator(args, values, 1);

    for (int i = -30; i <= 30; ++i)
    {
        double x = i / 10.0;
        EXPECT_NEAR(x * x, interpolator.evaluate(x), Error);
        EXPECT_NEAR(2 * x, interpolator.evaluateDerivative(x), Error);
        EXPECT_NEAR(2, interpolator.evaluateSecondDerivative(x), Error);
        EXPECT_NEAR(x / 2, interpolator.evaluateValueToDerivativeRatio(x), Error);
        EXPECT_NEAR(x, interpolator.evaluateDerivativeToSecondDerivativeRatio(x), Error);
    }
}

TEST(LagrangeInterpolator, Cubic)
{
    std::vector<double> args, values;
    args.push_back(1); values.push_back(cubicFunc(1));
    args.push_back(2); values.push_back(cubicFunc(2));
    args.push_back(3); values.push_back(cubicFunc(3));
    args.push_back(4); values.push_back(cubicFunc(4));

    LagrangeInterpolator<double> interpolator(args, values, 1);

    for (int i = -30; i <= 30; ++i)
    {
        double x = i / 10.0;
        EXPECT_NEAR(cubicFunc(x), interpolator.evaluate(x), Error);
        EXPECT_NEAR(cubicFunc1(x), interpolator.evaluateDerivative(x), Error);
        EXPECT_NEAR(cubicFunc2(x), interpolator.evaluateSecondDerivative(x), Error);
        EXPECT_NEAR(cubicFunc(x) / cubicFunc1(x), interpolator.evaluateValueToDerivativeRatio(x), Error);
        if (i != 5) // 12 * 5 / 10 - 6 = 0
        {
            EXPECT_NEAR(cubicFunc1(x) / cubicFunc2(x), interpolator.evaluateDerivativeToSecondDerivativeRatio(x), Error);
        }
    }
}

TEST(LagrangeInterpolator, Updates)
{
    std::vector<double> args;
    // y = x^2
    args.push_back(1);
    args.push_back(2);
    args.push_back(3);

    LagrangeInterpolator<double> interpolator(1);
    EXPECT_EQ(1, interpolator.scale());

    interpolator.updateArguments(args);
    interpolator.updateValue(0, 1);
    interpolator.updateValue(1, 4);
    interpolator.updateValue(2, 9);

    for (int i = -30; i <= 30; ++i)
    {
        double x = i / 10.0;
        EXPECT_NEAR(x * x, interpolator.evaluate(x), Error);
        EXPECT_NEAR(2 * x, interpolator.evaluateDerivative(x), Error);
        EXPECT_NEAR(2, interpolator.evaluateSecondDerivative(x), Error);
        EXPECT_NEAR(x / 2, interpolator.evaluateValueToDerivativeRatio(x), Error);
        EXPECT_NEAR(x, interpolator.evaluateDerivativeToSecondDerivativeRatio(x), Error);
    }
}
