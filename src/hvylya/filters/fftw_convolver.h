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

#include <hvylya/core/aligned_vector.h>
#include <hvylya/core/simd_vector.h>

#include <fftw3.h>

namespace hvylya {
namespace filters {

template <typename Input, typename Output>
class FftwConvolver
{
};

template <>
class FftwConvolver<float, float>
{
  public:
    FftwConvolver(const float* taps, std::size_t block_size, float* history, float* transformed_samples);

    ~FftwConvolver();

    void convolve(const float* input, float* output);

  private:
    typedef core::SimdVector<std::complex<float>, core::Aligned> ComplexVector;

    // Alignment is needed for vectorized version.
    core::AlignedVector<std::complex<float>> fft_taps_, fft_samples_;
    fftwf_plan fft_samples_plan_, fft_samples_back_plan_;
    const std::size_t block_size_, fft_size_;
};

template <>
class FftwConvolver<float, std::complex<float>>
{
  public:
    FftwConvolver(const std::complex<float>* taps, std::size_t block_size, float* history, std::complex<float>* transformed_samples);

    ~FftwConvolver();

    void convolve(const float* input, std::complex<float>* output);

  private:
    typedef core::SimdVector<std::complex<float>, core::Aligned> ComplexVector;
    typedef core::SimdVector<std::complex<float>, core::NonAligned> ComplexVectorNonAligned;

    // Alignment is needed for vectorized version.
    core::AlignedVector<std::complex<float>> fft_taps_, fft_samples_;
    fftwf_plan fft_samples_plan_, fft_samples_back_plan_;
    const std::size_t block_size_, fft_size_;
};

template <>
class FftwConvolver<std::complex<float>, std::complex<float>>
{
  public:
    FftwConvolver(const std::complex<float>* taps, std::size_t block_size, std::complex<float>* history, std::complex<float>* transformed_samples);

    ~FftwConvolver();

    void convolve(const std::complex<float>* input, std::complex<float>* output);

  private:
    typedef core::SimdVector<std::complex<float>, core::Aligned> ComplexVector;

    // Alignment is needed for vectorized version.
    core::AlignedVector<std::complex<float>> fft_taps_, fft_samples_;
    fftwf_plan fft_samples_plan_, fft_samples_back_plan_;
    const std::size_t block_size_, fft_size_;
};

} // namespace filters
} // namespace hvylya
