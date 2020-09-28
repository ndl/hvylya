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

static const std::size_t Iterations = 10 * TEST_LOAD_FACTOR;

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

TEST_F(FirFilterRegressionTestFloat, FmBasebandFloatLong)
{
    run(Iterations);
}

TEST_F(FirFilterRegressionTestComplexFloat, FmBasebandComplexFloatLong)
{
    run(Iterations);
}
