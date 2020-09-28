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

#include <hvylya/core/approx_trigonometry.h>

using namespace hvylya::core;

namespace {

const uint32_t atan2_sign_mask = 0x80000000;

// Approximates atan2(y, x) normalized to the [0,4) range
// with a maximum error of 0.1620 degrees.
const float atan2_b = 0.596227f;

// reinterpret_casts below are used to manipulate the signs.
DISABLE_WARNING_PUSH("-Wundefined-reinterpret-cast")

float normalized_atan2(float y, float x)
{
    // Extract the sign bits
    uint32_t ux_s  = atan2_sign_mask & reinterpret_cast<uint32_t&>(x);
    uint32_t uy_s  = atan2_sign_mask & reinterpret_cast<uint32_t&>(y);

    // Determine the quadrant offset
    float q = float((~ux_s & uy_s) >> 29 | ux_s >> 30); 

    // Calculate the arctangent in the first quadrant
    float bxy_a = std::abs(atan2_b * x * y);
    float num = bxy_a + y * y;
    float atan_1q =  num / (x * x + bxy_a + num);

    // Translate it to the proper quadrant
    uint32_t uatan_2q = (ux_s ^ uy_s) | reinterpret_cast<uint32_t&>(atan_1q);

    return q + reinterpret_cast<float&>(uatan_2q);
}

// Approximates atan2(y, x) normalized to the [0,4) range
// with a maximum error of 0.00811 degrees.
const float Sqrt17 = 4.1231056256176605498f;
const float atan2_c = (1.0f + Sqrt17) / 8.0f;
const float atan2_c1 = (1.0f + Sqrt17) / 8.0f + 1.0f;

float normalized_atan2_slower(float y, float x)
{
    // Extract the sign bits
    uint32_t ux_s  = atan2_sign_mask & reinterpret_cast<uint32_t&>(x);
    uint32_t uy_s  = atan2_sign_mask & reinterpret_cast<uint32_t&>(y);

    // Determine the quadrant offset
    float q = float((~ux_s & uy_s) >> 29 | ux_s >> 30); 

    // Calculate the arctangent in the first quadrant
    x = std::abs(x);
    y = std::abs(y);
    float xx = x * x;
    float yy = y * y;
    float xxx = xx * x;
    float yyy = yy * y;
    float yxx = y * xx;
    float yyx = yy * x;
    float atan_1q = (atan2_c * yxx + yyx + yyy) / (xxx + atan2_c1 * yxx + atan2_c1 * yyx + yyy);

    // Translate it to the proper quadrant
    uint32_t uatan_2q = (ux_s ^ uy_s) | reinterpret_cast<uint32_t&>(atan_1q);

    return q + reinterpret_cast<float&>(uatan_2q);
}

DISABLE_WARNING_POP()

#ifdef __SSSE3__
template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 16, Alignment> normalized_atan2(
    const SimdVectorImpl<std::complex<float>, 16, Alignment> arg0,
    const SimdVectorImpl<std::complex<float>, 16, Alignment> arg1
)
{
    // Decompose input into x and y parts.
    const __m128 x = __builtin_shufflevector(arg0.elements_, arg1.elements_, 0, 2, 4, 6);
    const __m128 y = __builtin_shufflevector(arg0.elements_, arg1.elements_, 1, 3, 5, 7);

    // Determine signs.
    const __m128i x_signs = _mm_srli_epi32(_mm_castps_si128(x), 31);
    const __m128i y_signs = _mm_srli_epi32(_mm_castps_si128(y), 31);

    // Prepare offsets.
    const __m128 quadrant = _mm_andnot_ps(
        _mm_castsi128_ps(_mm_sub_epi32(x_signs, _mm_set1_epi32(1))),
        _mm_set1_ps(-2.0f)
    );
    const __m128 dest_sign_mask = _mm_castsi128_ps(_mm_slli_epi32(_mm_xor_si128(x_signs, y_signs), 31));

    // Calculate the arctangent in the first quadrant.
    const __m128 bxy_a = _mm_set1_ps(atan2_b) * x * y;
    const __m128 abs_bxy_a = _mm_castsi128_ps(_mm_set1_epi32(atan2_sign_mask - 1) & _mm_castps_si128(bxy_a));
    const __m128 num = abs_bxy_a + y * y;
    const __m128 denom = x * x + abs_bxy_a + num;
    const __m128 zero_mask = _mm_cmpeq_ps(_mm_set1_ps(0.0f), denom);
    const __m128 atan_1q = _mm_andnot_ps(zero_mask, num / denom);

    // Final result calculation, based on x/y signs, is as follows:
    // 0th quad (0, 0): atan_1q
    // 1st quad (1, 0): -(-2 + atan_1q)
    // 2nd quad (0, 1): -atan_1q
    // 3rd quad (1, 1): (-2 + atan_1q)
    //
    // Return the result moved into the appropriate quadrant.
    return SimdVectorImpl<float, 16, Alignment>(_mm_xor_ps(quadrant + atan_1q, dest_sign_mask));
}
#endif // __SSSE3__

#ifdef __AVX2__
template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 32, Alignment> normalized_atan2(
    const SimdVectorImpl<std::complex<float>, 32, Alignment> arg0,
    const SimdVectorImpl<std::complex<float>, 32, Alignment> arg1
)
{
    // Decompose input into x and y parts.
    const __m256 x = __builtin_shufflevector(arg0.elements_, arg1.elements_, 0, 2, 4, 6, 8, 10, 12, 14);
    const __m256 y = __builtin_shufflevector(arg0.elements_, arg1.elements_, 1, 3, 5, 7, 9, 11, 13, 15);

    // Determine signs.
    const __m256i x_signs = _mm256_srli_epi32(_mm256_castps_si256(x), 31);
    const __m256i y_signs = _mm256_srli_epi32(_mm256_castps_si256(y), 31);

    // Prepare offsets.
    const __m256 quadrant = _mm256_andnot_ps(
        _mm256_castsi256_ps(_mm256_sub_epi32(x_signs, _mm256_set1_epi32(1))),
        _mm256_set1_ps(-2.0f)
    );
    const __m256 dest_sign_mask = _mm256_castsi256_ps(_mm256_slli_epi32(_mm256_xor_si256(x_signs, y_signs), 31));

    // Calculate the arctangent in the first quadrant.
    const __m256 bxy_a = _mm256_set1_ps(atan2_b) * x * y;
    const __m256 abs_bxy_a = _mm256_castsi256_ps(_mm256_set1_epi32(atan2_sign_mask - 1) & _mm256_castps_si256(bxy_a));
    const __m256 num = abs_bxy_a + y * y;
    const __m256 denom = x * x + abs_bxy_a + num;
    const __m256 zero_mask = _mm256_cmp_ps(_mm256_set1_ps(0.0f), denom, _CMP_EQ_OQ);
    const __m256 atan_1q = _mm256_andnot_ps(zero_mask, num / denom);

    // Final result calculation, based on x/y signs, is as follows:
    // 0th quad (0, 0): atan_1q
    // 1st quad (1, 0): -(-2 + atan_1q)
    // 2nd quad (0, 1): -atan_1q
    // 3rd quad (1, 1): (-2 + atan_1q)
    //
    // Return the result moved into the appropriate quadrant.
    return SimdVectorImpl<float, 32, Alignment>(_mm256_xor_ps(quadrant + atan_1q, dest_sign_mask));
}
#endif // __AVX2__

} // anonymous namespace

float hvylya::core::approx_normalized_atan2(float y, float x)
{
    if (x == 0.0f && y == 0.0f) return 0.0f;
    float result = normalized_atan2(y, x);
    return (result > 2.0f) ? result - 4.0f : result;
}

float hvylya::core::approx_atan2(float y, float x)
{
    return float(M_PI / 2) * approx_normalized_atan2(y, x);
}

float hvylya::core::approx_normalized_atan2_slower(float y, float x)
{
    if (x == 0.0f && y == 0.0f) return 0.0f;
    float result = normalized_atan2_slower(y, x);
    return (result > 2.0f) ? result - 4.0f : result;
}

float hvylya::core::approx_atan2_slower(float y, float x)
{
    return float(M_PI / 2) * approx_normalized_atan2_slower(y, x);
}

float hvylya::core::approx_cos(float x)
{
    float x2 = x * x;
    return
        ((((1.90652668840074246305e-05f * x2 -
            1.34410769349285321733e-03f) * x2 +
            4.15223086250910767516e-02f) * x2 -
            4.99837602272995734437e-01f) * x2 +
            9.99971094606182687341e-01f);
}

float hvylya::core::approx_sin(float x)
{
    float x2 = x * x;
    return
        ((((2.17326217498596729611e-06f * x2 -
            1.93162796407356830500e-04f) * x2 +
            8.31238887417884598346e-03f) * x2 -
            1.66632595072086745320e-01f) * x2 +
            9.99984594193494365437e-01f) * x;
}

template <std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<float, ByteSize, Alignment> ApproxTrigonometrySimd<ByteSize, Alignment>::normalized_atan2(
    const SimdVectorImpl<std::complex<float>, ByteSize, Alignment> arg0,
    const SimdVectorImpl<std::complex<float>, ByteSize, Alignment> arg1
)
{
    return ::normalized_atan2(arg0, arg1);
}

template <std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<float, ByteSize, Alignment> ApproxTrigonometrySimd<ByteSize, Alignment>::atan2(
    const SimdVectorImpl<std::complex<float>, ByteSize, Alignment> arg0,
    const SimdVectorImpl<std::complex<float>, ByteSize, Alignment> arg1
)
{
    return M_PI / 2 * ::normalized_atan2(arg0, arg1);
}

template <std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<float, ByteSize, Alignment> ApproxTrigonometrySimd<ByteSize, Alignment>::cos(const SimdVectorImpl<float, ByteSize, Alignment> x)
{
    const SimdVectorImpl<float, ByteSize, Alignment> x2 = x * x;
    return
        ((((float(1.90652668840074246305e-05) * x2 -
            float(1.34410769349285321733e-03)) * x2 +
            float(4.15223086250910767516e-02)) * x2 -
            float(4.99837602272995734437e-01)) * x2 +
            float(9.99971094606182687341e-01));
}

template <std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<float, ByteSize, Alignment> ApproxTrigonometrySimd<ByteSize, Alignment>::sin(const SimdVectorImpl<float, ByteSize, Alignment> x)
{
    const SimdVectorImpl<float, ByteSize, Alignment> x2 = x * x;
    return
        ((((float(2.17326217498596729611e-06) * x2 -
            float(1.93162796407356830500e-04)) * x2 +
            float(8.31238887417884598346e-03)) * x2 -
            float(1.66632595072086745320e-01)) * x2 +
            float(9.99984594193494365437e-01)) * x;
}

#ifdef __SSSE3__
template class hvylya::core::ApproxTrigonometrySimd<16, Aligned>;
template class hvylya::core::ApproxTrigonometrySimd<16, NonAligned>;
#endif // __SSSE3__

#ifdef __AVX2__
template class hvylya::core::ApproxTrigonometrySimd<32, Aligned>;
template class hvylya::core::ApproxTrigonometrySimd<32, NonAligned>;
#endif // __AVX2__
