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

enum class FirFilterType
{
    Type1 = 1,
    One = 1,
    First = 1,
    I = 1,

    Type2 = 2,
    Two = 2,
    Second = 2,
    II = 2,

    Type3 = 3,
    Three = 3,
    Third = 3,
    III = 3,

    Type4 = 4,
    Four = 4,
    Fourth = 4,
    IV = 4,

    Symmetric = 5,
    AntiSymmetric = 6
};

enum class FirTapsParity
{
    Odd = 1,
    Even = 2,
    Any = 3
};

template <typename T>
class PmFiltersDesigner: core::NonCopyable
{
  public:
    static bool createLowpassFilter(
        std::vector<T>& taps, 
        T passband,
        T stopband,
        T ripple_db,
        T attenuation_db,
        T gain = 1,
        FirFilterType type = FirFilterType::Symmetric,
        std::size_t alignment = 1
    );

    static bool createHighpassFilter(
        std::vector<T>& taps, 
        T stopband,
        T passband,
        T ripple_db,
        T attenuation_db,
        T gain = 1,
        FirFilterType type = FirFilterType(1),
        std::size_t alignment = 1
    );

    static bool createBandpassFilter(
        std::vector<T>& taps, 
        T left_stopband,
        T left_passband,
        T right_passband,
        T right_stopband,
        T ripple_db,
        T attenuation_db,
        T gain = 1,
        FirFilterType type = FirFilterType::Symmetric,
        std::size_t alignment = 1
    );

    static bool createHilbertBandpassTransform(
        std::vector<T>& taps, 
        T left_passband,
        T right_passband,
        T ripple_db,
        T gain = 1,
        std::size_t alignment = 1
    );

    static bool createHilbertHighpassTransform(
        std::vector<T>& taps, 
        T passband,
        T ripple_db,
        T gain = 1,
        std::size_t alignment = 1
    );

    static bool createDifferentiator(
        std::vector<T>& taps, 
        T passband,
        T stopband,
        T ripple_db,
        T attenuation_db,
        T gain = 1,
        FirTapsParity parity = FirTapsParity::Any,
        std::size_t alignment = 1
    );
};

} // namespace hvylya
} // namespace filters
