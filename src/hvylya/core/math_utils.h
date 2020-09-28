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

namespace hvylya {
namespace core {

template <typename T>
T roundDown(T value, T alignment)
{
    return (value / alignment) * alignment;
}

template <typename T>
T roundUp(T value, T alignment)
{
    return ((value + alignment - 1) / alignment) * alignment;
}

template <typename T>
T sqr(T value)
{
    return value * value;
}

template <typename T>
int sign(T value)
{
    if (value > 0)
    {
        return 1;
    }
    else if (value < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

inline std::size_t roundUpToPowerOfTwo(std::size_t value)
{
    return 1l << (sizeof(std::size_t) * CHAR_BIT - std::size_t(__builtin_clzl(value)));
}

template <typename T>
struct NumericTypeMapper
{
    typedef T ScalarType;
    typedef T ValueType;
};

template <typename T>
struct NumericTypeMapper<std::complex<T>>
{
    typedef T ScalarType;
    typedef std::complex<T> ValueType;
};

} // namespace common
} // namespace hvylya
