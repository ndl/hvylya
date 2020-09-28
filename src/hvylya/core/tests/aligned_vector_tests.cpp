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

#include <hvylya/core/aligned_vector.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::core;

TEST(AlignedVectorTests, Alignment)
{
    AlignedVector<float> vec(3);
    EXPECT_TRUE(isPointerAligned(&vec[0]));
}

TEST(AlignedVectorTests, Resize)
{
    AlignedVector<float> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(0, vec.size());

    vec.resize(3, 1.0f);
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(3, vec.size());
    EXPECT_EQ(1.0f, vec[2]);

    std::size_t new_size = AlignedVector<float>::AlignmentInElements + 1;
    vec.resize(new_size, 1.0f);
    EXPECT_EQ(new_size, vec.size());
    EXPECT_EQ(1.0f, vec[new_size - 1]);

    vec.clear();
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(0, vec.size());
}
