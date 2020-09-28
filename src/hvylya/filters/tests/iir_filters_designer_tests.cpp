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

#include <hvylya/filters/iir_filters_designer.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::core;
using namespace hvylya::filters;

namespace {

const std::size_t SamplesCount = 10000;
const double Accuracy = 1e-3;

template <class Filter>
float applyFilter(Filter filter_tmpl, float freq)
{
    Filter filter = filter_tmpl;
    float w(float(2 * M_PI * freq)), result(0);
    for (std::size_t i = 0; i < SamplesCount; ++i)
    {
        result += sqr(filter.filter(w != 0.0 ? std::sin(w * i) : 0.5f));
    }

    result /= SamplesCount;
    return result;
}
}

TEST(IirFiltersDesigner, LowpassFirstOrderFilter)
{
    std::tuple<float, float> TestData[] =
    {
        std::make_tuple(0, -6.03098),
        std::make_tuple(0.002, -3.18025),
        std::make_tuple(0.004, -3.65367),
        std::make_tuple(0.006, -4.34351),
        std::make_tuple(0.008, -5.15584),
        std::make_tuple(0.01, -6.01737),
        std::make_tuple(0.02, -10.0017),
        std::make_tuple(0.04, -15.3492),
        std::make_tuple(0.06, -18.7843),
        std::make_tuple(0.08, -21.3131),
        std::make_tuple(0.1, -23.3341),
        std::make_tuple(0.2, -30.2913),
        std::make_tuple(0.3, -35.835),
        std::make_tuple(0.4, -42.8224),
        std::make_tuple(0.4995, -87.7201)

    };

    IirFiltersDesigner<float>::IirFirstOrderFilter filter_tmpl = IirFiltersDesigner<float>::createLowpassFirstOrderFilter(float(0.01 * 2 * M_PI));

    for (std::size_t ind = 0; ind < sizeof(TestData) / sizeof(TestData[0]); ++ind)
    {
        float result = applyFilter(filter_tmpl, std::get<0>(TestData[ind]));
        EXPECT_NEAR(std::get<1>(TestData[ind]), 10 * std::log10(result), Accuracy);
    }
}

TEST(IirFiltersDesigner, LowpassBiquadFilter)
{
    std::tuple<float, float> TestData[] =
    {
        std::make_tuple(0.0, -6.03841),
        std::make_tuple(0.002, -3.11107),
        std::make_tuple(0.004, -3.63361),
        std::make_tuple(0.006, -4.97958),
        std::make_tuple(0.008, -7.13196),
        std::make_tuple(0.01, -9.64223),
        std::make_tuple(0.02, -20.2499),
        std::make_tuple(0.04, -32.0372),
        std::make_tuple(0.06, -38.9588),
        std::make_tuple(0.08, -43.8234),
        std::make_tuple(0.1, -47.5505),
        std::make_tuple(0.2, -58.8511),
        std::make_tuple(0.3, -66.0084),
        std::make_tuple(0.4, -73.2345),
        std::make_tuple(0.4995, -101.176)
    };

    IirFiltersDesigner<float>::IirBiquadFilter filter_tmpl = IirFiltersDesigner<float>::createLowpassBiquadFilter(float(0.0075 * 2 * M_PI), 2.0f);

    for (std::size_t ind = 0; ind < sizeof(TestData) / sizeof(TestData[0]); ++ind)
    {
        float result = applyFilter(filter_tmpl, std::get<0>(TestData[ind]));
        EXPECT_NEAR(std::get<1>(TestData[ind]), 10 * std::log10(result), Accuracy);
    }
}
