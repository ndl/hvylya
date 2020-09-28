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

#include <hvylya/core/simd_vector16.h>
#include <hvylya/core/simd_vector32.h>

namespace hvylya {
namespace core {

template <typename DstVectorType, SimdAlignmentType Alignment>
std::array<DstVectorType, 2> castVectorImpl(SimdVectorImpl<float, 32, Alignment> arg, SimdVectorImpl<double, 32, Alignment> /*unused*/)
{
    return std::array<DstVectorType, 2>(
        {
            _mm256_cvtps_pd(__builtin_shufflevector(arg.elements_, arg.elements_, 0, 1, 2, 3)),
            _mm256_cvtps_pd(__builtin_shufflevector(arg.elements_, arg.elements_, 4, 5, 6, 7))
        }
    );
}

template <SimdAlignmentType Alignment>
float sum(const SimdVectorImpl<float, 32, Alignment> arg)
{
    float result;
    __m256 perm = _mm256_permute2f128_ps(arg.elements_, arg.elements_, 1);
    __m256 sum = _mm256_add_ps(arg.elements_, perm);
    sum = _mm256_hadd_ps(sum, sum);
    sum = _mm256_hadd_ps(sum, sum);
    _mm_store_ss(&result, _mm256_castps256_ps128(sum));
    return result;
}

template <SimdAlignmentType Alignment>
std::complex<float> sum(const SimdVectorImpl<std::complex<float>, 32, Alignment> arg)
{
    __m256 sum = __builtin_shufflevector(arg.elements_, arg.elements_, 0, 2, 4, 6, 1, 3, 5, 7);
    // After execution, sum = 0 + 2, 4 + 6, 0 + 2, 4 + 6, 1 + 3, 5 + 7, 1 + 3, 5 + 7
    sum = _mm256_hadd_ps(sum, sum);
    // After execution, sum = 0 + 2 + 4 + 6 (x4), 1 + 3 + 5 + 7 (x4)
    sum = _mm256_hadd_ps(sum, sum);
    return std::complex<float>(sum[0], sum[4]);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 32, Alignment> abs(const SimdVectorImpl<float, 32, Alignment> arg)
{
    return _mm256_andnot_ps(_mm256_set1_ps(-0.0f), arg.elements_);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 32, Alignment> fusedMultiplyAdd(const SimdVectorImpl<float, 32, Alignment> arg0, const SimdVectorImpl<float, 32, Alignment> arg1, const SimdVectorImpl<float, 32, Alignment> arg2)
{
    return SimdVectorImpl<float, 32, Alignment>(_mm256_fmadd_ps(arg0.elements_, arg1.elements_, arg2.elements_));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 32, Alignment> operator *(const SimdVectorImpl<std::complex<float>, 32, Alignment> arg0, const SimdVectorImpl<std::complex<float>, 32, Alignment> arg1)
{
    __m256 arg0_swap = __builtin_shufflevector(arg0.elements_, arg0.elements_, 1, 0, 3, 2, 5, 4, 7, 6);
    __m256 arg1_real = __builtin_shufflevector(arg1.elements_, arg1.elements_, 0, 0, 2, 2, 4, 4, 6, 6);
    __m256 arg1_imag = __builtin_shufflevector(arg1.elements_, arg1.elements_, 1, 1, 3, 3, 5, 5, 7, 7);
    return SimdVectorImpl<std::complex<float>, 32, Alignment>(_mm256_addsub_ps(arg0.elements_ * arg1_real, arg0_swap * arg1_imag));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 32, Alignment> multiplyConjugated(const SimdVectorImpl<std::complex<float>, 32, Alignment> arg0, const SimdVectorImpl<std::complex<float>, 32, Alignment> arg1)
{
    __m256 arg0_swap = __builtin_shufflevector(arg0.elements_, arg0.elements_, 1, 0, 3, 2, 5, 4, 7, 6);
    __m256 arg1_real = __builtin_shufflevector(arg1.elements_, arg1.elements_, 0, 0, 2, 2, 4, 4, 6, 6);
    __m256 arg1_imag = -__builtin_shufflevector(arg1.elements_, arg1.elements_, 1, 1, 3, 3, 5, 5, 7, 7);
    return SimdVectorImpl<std::complex<float>, 32, Alignment>(_mm256_addsub_ps(arg0.elements_ * arg1_real, arg0_swap * arg1_imag));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 32, Alignment> fusedMultiplyAdd(const SimdVectorImpl<std::complex<float>, 32, Alignment> arg0, const typename SimdVectorImpl<std::complex<float>, 32, Alignment>::FlattenedType arg1, const SimdVectorImpl<std::complex<float>, 32, Alignment> arg2)
{
    return SimdVectorImpl<std::complex<float>, 32, Alignment>(_mm256_fmadd_ps(arg0.elements_, arg1.elements_, arg2.elements_));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 32, Alignment> fusedMultiplyAdd(const typename SimdVectorImpl<std::complex<float>, 32, Alignment>::FlattenedType arg0, const SimdVectorImpl<std::complex<float>, 32, Alignment> arg1, const SimdVectorImpl<std::complex<float>, 32, Alignment> arg2)
{
    return SimdVectorImpl<std::complex<float>, 32, Alignment>(_mm256_fmadd_ps(arg0.elements_, arg1.elements_, arg2.elements_));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<double>, 32, Alignment> fusedMultiplyAdd(const SimdVectorImpl<std::complex<double>, 32, Alignment> arg0, const typename SimdVectorImpl<std::complex<double>, 32, Alignment>::FlattenedType arg1, const SimdVectorImpl<std::complex<double>, 32, Alignment> arg2)
{
    return SimdVectorImpl<std::complex<double>, 32, Alignment>(_mm256_fmadd_pd(arg0.elements_, arg1.elements_, arg2.elements_));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<double>, 32, Alignment> fusedMultiplyAdd(const typename SimdVectorImpl<std::complex<double>, 32, Alignment>::FlattenedType arg0, const SimdVectorImpl<std::complex<double>, 32, Alignment> arg1, const SimdVectorImpl<std::complex<double>, 32, Alignment> arg2)
{
    return SimdVectorImpl<std::complex<double>, 32, Alignment>(_mm256_fmadd_pd(arg0.elements_, arg1.elements_, arg2.elements_));
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 32, Alignment> conjugate(const SimdVectorImpl<std::complex<float>, 32, Alignment> arg)
{
    const __m256i conj_mask =
    {
        static_cast<long long>(0x8000000000000000ull),
        static_cast<long long>(0x8000000000000000ull),
        static_cast<long long>(0x8000000000000000ull),
        static_cast<long long>(0x8000000000000000ull)
    };
    return SimdVectorImpl<std::complex<float>, 32, Alignment>(_mm256_xor_ps(arg.elements_, _mm256_castsi256_ps(conj_mask)));
}

} // namespace core
} // namespace hvylya
