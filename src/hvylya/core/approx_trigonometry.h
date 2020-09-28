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

#include <hvylya/core/simd_vector.h>

namespace hvylya {
namespace core {

float approx_normalized_atan2(float y, float x);

float approx_atan2(float y, float x);

float approx_normalized_atan2_slower(float y, float x);

float approx_atan2_slower(float y, float x);

float approx_cos(float x);

float approx_sin(float x);

template <std::size_t ByteSize, SimdAlignmentType Alignment>
class ApproxTrigonometrySimd
{
  public:
    static_assert(ByteSize == 16 || ByteSize == 32, "Only 16 and 32 bytes sizes are supported for ApproxTrigonometrySimd");

    static SimdVectorImpl<float, ByteSize, Alignment> normalized_atan2(
        const SimdVectorImpl<std::complex<float>, ByteSize, Alignment> arg0,
	const SimdVectorImpl<std::complex<float>, ByteSize, Alignment> arg1
    );

    static SimdVectorImpl<float, ByteSize, Alignment> atan2(
        const SimdVectorImpl<std::complex<float>, ByteSize, Alignment> arg0,
	const SimdVectorImpl<std::complex<float>, ByteSize, Alignment> arg1
    );

    static SimdVectorImpl<float, ByteSize, Alignment> cos(
        const SimdVectorImpl<float, ByteSize, Alignment> x
    );

    static SimdVectorImpl<float, ByteSize, Alignment> sin(
        const SimdVectorImpl<float, ByteSize, Alignment> x
    );
};

template <typename Vector>
class ApproxTrigonometrySimdFromVector
{
};

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
class ApproxTrigonometrySimdFromVector<SimdVectorImpl<T, ByteSize, Alignment>>: public ApproxTrigonometrySimd<ByteSize, Alignment>
{
};

template <typename T, SimdAlignmentType Alignment>
class ApproxTrigonometrySimdFromVector<SimdVector<T, Alignment>>: public ApproxTrigonometrySimd<MaxSimdByteSize, Alignment>
{
};

} // namespace core
} // namespace hvylya
