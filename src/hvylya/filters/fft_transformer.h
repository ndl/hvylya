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

#include <fftw3.h>

namespace hvylya {
namespace filters {

template <typename Input, typename Output>
class FftTransformer
{
};

template <>
class FftTransformer<float, std::complex<float>>
{
  public:
    FftTransformer(std::size_t fft_size);

    ~FftTransformer();

    void transform(const float* input, std::complex<float>* output);

  private:
    fftwf_plan fft_plan_;
    const std::size_t fft_size_;
};

template <>
class FftTransformer<std::complex<float>, std::complex<float>>
{
  public:
    FftTransformer(std::size_t fft_size);

    ~FftTransformer();

    void transform(const std::complex<float>* input, std::complex<float>* output);

  private:
    fftwf_plan fft_plan_;
    const std::size_t fft_size_;
};

} // namespace filters
} // namespace hvylya
