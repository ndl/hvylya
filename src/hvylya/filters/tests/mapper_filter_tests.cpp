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

#include <hvylya/filters/mapper_filter.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::core;
using namespace hvylya::filters;

namespace {

void func1(const int& in, float& out)
{
    out = 2.0f * in;
}

}

TEST(MapperFilter, NormalFunction)
{
    AlignedVector<int> input(2);
    AlignedVector<float> output(2);

    Slice<int> input_slice(input);
    Slice<float> output_slice(output);

    auto input_tuple = std::make_tuple(std::cref(input_slice));
    auto output_tuple = std::make_tuple(std::ref(output_slice));

    MapperFilter<decltype(&func1)> mapper_filter(&func1);

    input[0] = 1;
    input[1] = 2;

    mapper_filter.process(input_tuple, output_tuple);

    EXPECT_EQ(2.0f, output[0]);
    EXPECT_EQ(4.0f, output[1]);
}

TEST(MapperFilter, Lambda)
{
    AlignedVector<float> input0(2), input1(2), output0(2), output1(2);

    Slice<float> input_slice0(input0), input_slice1(input1), output_slice0(output0), output_slice1(output1);

    auto input_tuple = std::make_tuple(std::cref(input_slice0), std::cref(input_slice1));
    auto output_tuple = std::make_tuple(std::ref(output_slice0), std::ref(output_slice1));

    auto mapper_filter(
        makeMapperFilter(
            [](const float& in0, const float& in1, float& out0, float& out1)
            {
                out0 = in0 + in1;
                out1 = in0 * in1;
            }
        )
    );

    input0[0] = 1.0f;
    input0[1] = 2.0f;

    input1[0] = 3.0f;
    input1[1] = 4.0f;

    mapper_filter.process(input_tuple, output_tuple);

    EXPECT_EQ(4.0f, output0[0]);
    EXPECT_EQ(6.0f, output0[1]);

    EXPECT_EQ(3.0f, output1[0]);
    EXPECT_EQ(8.0f, output1[1]);
}
