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

#include <hvylya/filters/rotator.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename T>
Rotator<T>::Rotator():
    storage_(5 * ComplexVector::Elements + 1),
    rotations_(*reinterpret_cast<ComplexVector*>(&storage_[0])),
    rotations_incremental_(*reinterpret_cast<ComplexVector*>(&storage_[ComplexVector::Elements])),
    rotations_scalar_step_(*reinterpret_cast<ComplexVector*>(&storage_[2 * ComplexVector::Elements])),
    rotations_vector_step_(*reinterpret_cast<ComplexVector*>(&storage_[3 * ComplexVector::Elements])),
    rotations_count_(0)
{
}

template <typename T>
Rotator<T>::Rotator(T center_frequency, std::size_t decimation_rate):
    Rotator()
{
    setFrequency(center_frequency, decimation_rate);
}

template <typename T>
void Rotator<T>::setFrequency(T center_frequency, std::size_t decimation_rate)
{
    center_frequency_ = center_frequency;
    decimation_rate_ = decimation_rate;
    reset();
}

template <typename T>
void Rotator<T>::createTaps(std::complex<T> new_taps[], const T taps[], std::size_t taps_count)
{
    T freq_shift = T(2 * M_PI * center_frequency_);

    for (std::size_t i = 0; i < taps_count; ++i)
    {
        T phase = i * freq_shift;
        new_taps[i] = taps[i] * std::complex<T>(std::cos(phase), std::sin(phase));
    }
}

template <typename T>
void Rotator<T>::reset()
{
    T freq_shift = T(2 * M_PI * center_frequency_);
    std::complex<T>* rotations = &storage_[4 * ComplexVector::Elements];

    rotations_count_ = 0;

    for (std::size_t i = 0; i < ComplexVector::Elements + 1; ++i)
    {
        T phase = -(i * decimation_rate_ * freq_shift);
        rotations[i] = std::complex<T>(std::cos(phase), std::sin(phase));
    }

    rotations_incremental_ = *reinterpret_cast<ComplexVector*>(&rotations[0]);
    rotations_scalar_step_ = ComplexVector(rotations[1]);
    rotations_vector_step_ = ComplexVector(rotations[ComplexVector::Elements]);
    rotations_ = createRotations(rotations[0]);
}

template <typename T>
void Rotator<T>::rotate(std::complex<T>* data, std::size_t data_size)
{
    ComplexVector rotations = rotations_;

    for (std::size_t index = 0; index < data_size;)
    {
        std::size_t size = std::min(data_size - index, RotationsBeforeReNormalization - rotations_count_), end_index = index + size;

        if (isPointerAligned(&data[index]))
        {
            for (; index + ComplexVector::Elements <= end_index; index += ComplexVector::Elements)
            {
                *reinterpret_cast<ComplexVector*>(&data[index]) *= rotations;
                rotations *= rotations_vector_step_;
            }
        }

        // Handle any left-overs if the input size wasn't aligned with ComplexVector::Elements.
        for (; index < end_index; ++index)
        {
            data[index] *= rotations[0];
            rotations *= rotations_scalar_step_;
        }

        rotations_count_ += size;
        if (rotations_count_ >= RotationsBeforeReNormalization)
        {
            rotations_count_ = 0;
            rotations = createRotations(rotations[0]);
        }
    }

    rotations_ = rotations;
}

template <typename T>
typename Rotator<T>::ComplexVector
Rotator<T>::createRotations(std::complex<T> last_rotation)
{
    return ComplexVector(last_rotation / std::abs(last_rotation)) * rotations_incremental_;
}

template class hvylya::filters::Rotator<float>;
