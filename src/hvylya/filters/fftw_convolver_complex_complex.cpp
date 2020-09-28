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

#include <hvylya/filters/fftw_convolver.h>

using namespace hvylya::core;
using namespace hvylya::filters;

FftwConvolver<std::complex<float>, std::complex<float>>::FftwConvolver(
    const std::complex<float>* taps,
    std::size_t block_size,
    std::complex<float>* history,
    std::complex<float>* transformed_samples
):
    block_size_(block_size),
    fft_size_(block_size)
{
    fft_taps_.resize(fft_size_);
    fft_samples_.resize(fft_size_);

    fftwf_plan fft_taps_plan =
        fftwf_plan_dft_1d(
            int(block_size_),
            reinterpret_cast<fftwf_complex*>(const_cast<std::complex<float>*>(taps)),
            reinterpret_cast<fftwf_complex*>(&fft_taps_[0]),
            FFTW_FORWARD,
            FFTW_ESTIMATE
        );
    fftwf_execute(fft_taps_plan);
    fftwf_destroy_plan(fft_taps_plan);

    // Could be vectorized, but it's executed just once - so not a big deal.
    float scale = 1.0f / block_size_;
    for (std::size_t i = 0; i < fft_size_; ++i)
    {
        fft_taps_[i] *= scale;
    }

    fft_samples_plan_ =
        fftwf_plan_dft_1d(
            int(block_size_),
            reinterpret_cast<fftwf_complex*>(history),
            reinterpret_cast<fftwf_complex*>(&fft_samples_[0]),
            FFTW_FORWARD,
            FFTW_ESTIMATE
        );

    fft_samples_back_plan_ = 
        fftwf_plan_dft_1d(
            int(block_size_),
            reinterpret_cast<fftwf_complex*>(&fft_samples_[0]),
            reinterpret_cast<fftwf_complex*>(transformed_samples),
            FFTW_BACKWARD,
            FFTW_ESTIMATE
        );
}

FftwConvolver<std::complex<float>, std::complex<float>>::~FftwConvolver()
{
    fftwf_destroy_plan(fft_samples_back_plan_);
    fftwf_destroy_plan(fft_samples_plan_);
}

void FftwConvolver<std::complex<float>, std::complex<float>>::convolve(const std::complex<float>* input, std::complex<float>* output)
{
    fftwf_execute_dft(
        fft_samples_plan_,
        reinterpret_cast<fftwf_complex*>(const_cast<std::complex<float>*>(input)),
        reinterpret_cast<fftwf_complex*>(&fft_samples_[0])
    );

    for (std::size_t index = 0; index < fft_size_; index += ComplexVector::Elements)
    {
        *reinterpret_cast<ComplexVector*>(&fft_samples_[index]) *= *reinterpret_cast<ComplexVector*>(&fft_taps_[index]);
    }

    fftwf_execute_dft(
        fft_samples_back_plan_,
        reinterpret_cast<fftwf_complex*>(&fft_samples_[0]),
        reinterpret_cast<fftwf_complex*>(output)
    );
}
