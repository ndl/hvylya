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

namespace hvylya {
namespace core {

template <>
struct SimdVectorTraits<float, 16, Aligned>
{
  typedef float __attribute__((vector_size(16), aligned(16))) VectorType;
};

template <>
struct SimdVectorTraits<float, 16, NonAligned>
{
  typedef float __attribute__((vector_size(16), aligned(1))) VectorType;
};

template <SimdAlignmentType Alignment>
class SimdVectorImpl<float, 16, Alignment>: public SimdVectorIndexed<float, float, 16, Alignment>
{
  public:
    using SimdVectorIndexed<float, float, 16, Alignment>::SimdVectorIndexed;
    using SimdVectorIndexed<float, float, 16, Alignment>::elements_;

    SimdVectorImpl() { }

    SimdVectorImpl(float value)
    {
        elements_ = { value, value, value, value };
    }
};

template <SimdAlignmentType Alignment>
class SimdVectorImpl<std::complex<float>, 16, Alignment>: public SimdVectorIndexed<float, std::complex<float>, 16, Alignment>
{
  public:
    using SimdVectorIndexed<float, std::complex<float>, 16, Alignment>::SimdVectorIndexed;
    using SimdVectorIndexed<float, std::complex<float>, 16, Alignment>::elements_;

    SimdVectorImpl() { }

    SimdVectorImpl(std::complex<float> value)
    {
        const float real = value.real();
        const float imag = value.imag();
        elements_ = { real, imag, real, imag };
    }
};

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 8, Alignment> real(const SimdVectorImpl<std::complex<float>, 16, Alignment> arg)
{
    return __builtin_shufflevector(arg.elements_, arg.elements_, 0, 2);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 8, Alignment> imag(const SimdVectorImpl<std::complex<float>, 16, Alignment> arg)
{
    return __builtin_shufflevector(arg.elements_, arg.elements_, 1, 3);
}

template <SimdAlignmentType Alignment>
void splitComplex(
    const SimdVectorImpl<std::complex<float>, 16, Alignment> arg0,
    const SimdVectorImpl<std::complex<float>, 16, Alignment> arg1,
    SimdVectorImpl<float, 16, Alignment>& real,
    SimdVectorImpl<float, 16, Alignment>& imag
)
{
    real = __builtin_shufflevector(arg0.elements_, arg1.elements_, 0, 2, 4, 6);
    imag = __builtin_shufflevector(arg0.elements_, arg1.elements_, 1, 3, 5, 7);
}

template <SimdAlignmentType Alignment>
void mergeComplex(
    const SimdVectorImpl<float, 16, Alignment> real,
    const SimdVectorImpl<float, 16, Alignment> imag,
    SimdVectorImpl<std::complex<float>, 16, Alignment>& arg0,
    SimdVectorImpl<std::complex<float>, 16, Alignment>& arg1
)
{
    arg0 = __builtin_shufflevector(real.elements_, imag.elements_, 0, 4, 1, 5);
    arg1 = __builtin_shufflevector(real.elements_, imag.elements_, 2, 6, 3, 7);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 16, Alignment> flip(const SimdVectorImpl<std::complex<float>, 16, Alignment> arg)
{
    return SimdVectorImpl<std::complex<float>, 16, Alignment>(__builtin_shufflevector(arg.elements_, arg.elements_, 2, 3, 0, 1));
}

template <SimdAlignmentType Alignment>
struct AdjacentExtender<SimdVectorImpl<float, 16, Alignment>, SimdVectorImpl<float, 8, Alignment>>
{
    static SimdVectorImpl<float, 16, Alignment> extend(const SimdVectorImpl<float, 8, Alignment> arg)
    {
        return SimdVectorImpl<float, 16, Alignment>(__builtin_shufflevector(arg.elements_, arg.elements_, 0, 0, 1, 1));
    }
};

} // namespace core
} // namespace hvylya
