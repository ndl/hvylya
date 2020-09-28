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

#include <hvylya/core/common.h>

#include <gtest/gtest.h>

template <std::size_t Number>
struct NumberWrapperType
{
    enum: std::size_t { Value = Number };
};

#ifdef __AVX2__
typedef ::testing::Types<NumberWrapperType<16>, NumberWrapperType<32>> SimdByteSizes;
#else
typedef ::testing::Types<NumberWrapperType<16>> SimdByteSizes;
#endif

// gtest macros act as global constructors,
// so we have to disable this globally for all tests.
DISABLE_WARNING("-Wglobal-constructors")

// Type parameterized tests trigger this warning.
DISABLE_WARNING_PUSH("-Wmissing-variable-declarations")
