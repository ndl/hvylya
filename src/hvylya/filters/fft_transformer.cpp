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

#include <hvylya/filters/fft_transformer.h>

using namespace hvylya::core;
using namespace hvylya::filters;

FftTransformer<float, std::complex<float>>::FftTransformer(
    std::size_t fft_size
):
    fft_size_(fft_size)
{
    fft_plan_ =
        fftwf_plan_dft_r2c_1d(
            int(fft_size_),
            nullptr,
            nullptr,
            FFTW_ESTIMATE
        );
}

FftTransformer<float, std::complex<float>>::~FftTransformer()
{
    fftwf_destroy_plan(fft_plan_);
}

void FftTransformer<float, std::complex<float>>::transform(const float* input, std::complex<float>* output)
{
    fftwf_execute_dft_r2c(
        fft_plan_,
        const_cast<float*>(input),
        reinterpret_cast<fftwf_complex*>(output)
    );
}

FftTransformer<std::complex<float>, std::complex<float>>::FftTransformer(
    std::size_t fft_size
):
    fft_size_(fft_size)
{
    fft_plan_ =
        fftwf_plan_dft_1d(
            int(fft_size_),
            nullptr,
            nullptr,
            FFTW_FORWARD,
            FFTW_ESTIMATE
        );
}

FftTransformer<std::complex<float>, std::complex<float>>::~FftTransformer()
{
    fftwf_destroy_plan(fft_plan_);
}

void FftTransformer<std::complex<float>, std::complex<float>>::transform(const std::complex<float>* input, std::complex<float>* output)
{
    fftwf_execute_dft(
        fft_plan_,
        const_cast<fftwf_complex*>(reinterpret_cast<const fftwf_complex*>(input)),
        reinterpret_cast<fftwf_complex*>(output)
    );
}
