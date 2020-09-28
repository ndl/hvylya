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
namespace filters {

template <typename SampleType, typename TapType>
struct FilterTypeMapper { };

template <typename T>
struct FilterTypeMapper<T, T>
{
    typedef T ScalarType;
    typedef T SampleType;
    typedef T TapType;
    typedef T ResultType;
};

template <typename T>
struct FilterTypeMapper<std::complex<T>, T>
{
    typedef T ScalarType;
    typedef std::complex<T> SampleType;
    typedef T TapType;
    typedef std::complex<T> ResultType;
};

template <typename T>
struct FilterTypeMapper<T, std::complex<T>>
{
    typedef T ScalarType;
    typedef T SampleType;
    typedef std::complex<T> TapType;
    typedef std::complex<T> ResultType;
};

template <typename T>
struct FilterTypeMapper<std::complex<T>, std::complex<T>>
{
    typedef T ScalarType;
    typedef std::complex<T> SampleType;
    typedef std::complex<T> TapType;
    typedef std::complex<T> ResultType;
};

} // namespace filters
} // namespace hvylya
