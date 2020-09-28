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

#include <hvylya/core/simd_vector8.h>
#include <hvylya/core/simd_vector16.h>

namespace hvylya {
namespace core {

template <typename DstVectorType, SimdAlignmentType Alignment>
std::array<DstVectorType, 2> castVectorImpl(SimdVectorImpl<float, 16, Alignment> arg, SimdVectorImpl<double, 16, Alignment> /*unused*/)
{
    return std::array<DstVectorType, 2>(
        {
            _mm_cvtps_pd(__builtin_shufflevector(arg.elements_, arg.elements_, 0, 1)),
            _mm_cvtps_pd(__builtin_shufflevector(arg.elements_, arg.elements_, 2, 3))
        }
    );
}

template <SimdAlignmentType Alignment>
float sum(const SimdVectorImpl<float, 16, Alignment> arg)
{
    float result;
    __m128 vec = _mm_hadd_ps(arg.elements_, arg.elements_);
    __m128 vec2 = _mm_hadd_ps(vec, vec);
    _mm_store_ss(&result, vec2);
    return result;
}

template <SimdAlignmentType Alignment>
std::complex<float> sum(const SimdVectorImpl<std::complex<float>, 16, Alignment> arg)
{
    __m128 sum = __builtin_shufflevector(arg.elements_, arg.elements_, 0, 2, 1, 3);
    __m128 sum2 = _mm_hadd_ps(sum, sum);
    return std::complex<float>(sum2[0], sum2[1]);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 16, Alignment> normalized(SimdVectorImpl<std::complex<float>, 16, Alignment> arg)
{
    // After execution, mag = real^2, imag^2, real^2, imag^2
    __m128 mag = _mm_mul_ps(arg.elements_, arg.elements_);
    // After execution, mag = amp^2, amp^2, amp^2, amp^2
    mag = _mm_hadd_ps(mag, mag);

    // After execution, norm = 1 / amp, 1 / amp, 1 / amp, 1 / amp
    __m128 norm = _mm_rsqrt_ps(mag);
    // After execution, result = real / amp, imag / amp, real / amp, imag / amp
    __m128 result = _mm_mul_ps(arg.elements_, norm);

    return SimdVectorImpl<std::complex<float>, 16, Alignment>(result);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 16, Alignment> norm2(const SimdVectorImpl<std::complex<float>, 16, Alignment> arg)
{
    __m128 mag2 = _mm_mul_ps(arg.elements_, arg.elements_);
    return SimdVectorImpl<float, 16, Alignment>(_mm_hadd_ps(mag2, mag2));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 16, Alignment> rsqrt(const SimdVectorImpl<float, 16, Alignment> arg)
{
    return SimdVectorImpl<float, 16, Alignment>(_mm_rsqrt_ps(arg.elements_));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 16, Alignment> refinedRsqrt(const SimdVectorImpl<float, 16, Alignment> arg, const SimdVectorImpl<float, 16, Alignment> inv)
{
    return SimdVectorImpl<float, 16, Alignment>((0.5f * inv) * (3.0f - (arg * inv) * inv));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 16, Alignment> abs(const SimdVectorImpl<std::complex<float>, 16, Alignment> arg)
{
    SimdVectorImpl<float, 16, Alignment> val = norm2(arg);
    return val * refinedRsqrt(val, rsqrt(val));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 16, Alignment> abs(const SimdVectorImpl<float, 16, Alignment> arg)
{
    return _mm_andnot_ps(_mm_set1_ps(-0.0f), arg.elements_);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 16, Alignment> fusedMultiplyAdd(const SimdVectorImpl<float, 16, Alignment> arg0, const SimdVectorImpl<float, 16, Alignment> arg1, const SimdVectorImpl<float, 16, Alignment> arg2)
{
    // FMA is not available in SSSE3, revert to separate operations.
    return SimdVectorImpl<float, 16, Alignment>(arg0.elements_ * arg1.elements_ + arg2.elements_);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 16, Alignment> operator *(const SimdVectorImpl<std::complex<float>, 16, Alignment> arg0, const SimdVectorImpl<std::complex<float>, 16, Alignment> arg1)
{
    __m128 arg0_swap = __builtin_shufflevector(arg0.elements_, arg0.elements_, 1, 0, 3, 2);
    __m128 arg1_real = __builtin_shufflevector(arg1.elements_, arg1.elements_, 0, 0, 2, 2);
    __m128 arg1_imag = __builtin_shufflevector(arg1.elements_, arg1.elements_, 1, 1, 3, 3);
    return SimdVectorImpl<std::complex<float>, 16, Alignment>(_mm_addsub_ps(arg0.elements_ * arg1_real, arg0_swap * arg1_imag));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 16, Alignment> multiplyConjugated(const SimdVectorImpl<std::complex<float>, 16, Alignment> arg0, const SimdVectorImpl<std::complex<float>, 16, Alignment> arg1)
{
    __m128 arg0_swap = __builtin_shufflevector(arg0.elements_, arg0.elements_, 1, 0, 3, 2);
    __m128 arg1_real = __builtin_shufflevector(arg1.elements_, arg1.elements_, 0, 0, 2, 2);
    __m128 arg1_imag = -__builtin_shufflevector(arg1.elements_, arg1.elements_, 1, 1, 3, 3);
    return SimdVectorImpl<std::complex<float>, 16, Alignment>(_mm_addsub_ps(arg0.elements_ * arg1_real, arg0_swap * arg1_imag));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 16, Alignment> fusedMultiplyAdd(const SimdVectorImpl<std::complex<float>, 16, Alignment> arg0, const typename SimdVectorImpl<std::complex<float>, 16, Alignment>::FlattenedType arg1, const SimdVectorImpl<std::complex<float>, 16, Alignment> arg2)
{
    // FMA is not available in SSSE3, revert to separate operations.
    return SimdVectorImpl<std::complex<float>, 16, Alignment>(arg0.elements_ * arg1.elements_ + arg2.elements_);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 16, Alignment> fusedMultiplyAdd(const typename SimdVectorImpl<std::complex<float>, 16, Alignment>::FlattenedType arg0, const SimdVectorImpl<std::complex<float>, 16, Alignment> arg1, const SimdVectorImpl<std::complex<float>, 16, Alignment> arg2)
{
    // FMA is not available in SSSE3, revert to separate operations.
    return SimdVectorImpl<std::complex<float>, 16, Alignment>(arg0.elements_ * arg1.elements_ + arg2.elements_);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 16, Alignment> conjugate(const SimdVectorImpl<std::complex<float>, 16, Alignment> arg)
{
    const __m128i conj_mask =
    {
        static_cast<long long>(0x8000000000000000ull),
        static_cast<long long>(0x8000000000000000ull)
    };
    return SimdVectorImpl<std::complex<float>, 16, Alignment>(_mm_xor_ps(arg.elements_, _mm_castsi128_ps(conj_mask)));
}

} // namespace core
} // namespace hvylya
