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

namespace hvylya {
namespace filters {

template <typename T>
class Rotator
{
  public:
    Rotator();

    Rotator(T center_frequency, std::size_t decimation_rate = 1);

    void setFrequency(T center_frequency, std::size_t decimation_rate = 1);

    void createTaps(std::complex<T> new_taps[], const T taps[], std::size_t taps_count);

    void reset();

    void rotate(std::complex<T>* data, std::size_t data_size);

  private:
    typedef core::SimdVector<std::complex<T>, core::Aligned> ComplexVector;

    enum: std::size_t
    {
        RotationsBeforeReNormalization = 256 * ComplexVector::Elements
    };

    core::AlignedVector<std::complex<T>> storage_;
    ComplexVector &rotations_, &rotations_incremental_, &rotations_scalar_step_, &rotations_vector_step_;
    T center_frequency_;
    std::size_t rotations_count_, decimation_rate_;

    ComplexVector createRotations(std::complex<T> last_rotation);
};

} // namespace filters
} // namespace hvylya
