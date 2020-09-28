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
struct SimdVectorTraits<float, 32, Aligned>
{
  typedef float __attribute__((vector_size(32), aligned(32))) VectorType;
};

template <>
struct SimdVectorTraits<float, 32, NonAligned>
{
  typedef float __attribute__((vector_size(32), aligned(1))) VectorType;
};

template <>
struct SimdVectorTraits<double, 32, Aligned>
{
  typedef double __attribute__((vector_size(32), aligned(32))) VectorType;
};

template <>
struct SimdVectorTraits<double, 32, NonAligned>
{
  typedef double __attribute__((vector_size(32), aligned(1))) VectorType;
};

template <SimdAlignmentType Alignment>
class SimdVectorImpl<float, 32, Alignment>: public SimdVectorIndexed<float, float, 32, Alignment>
{
  public:
    using SimdVectorIndexed<float, float, 32, Alignment>::SimdVectorIndexed;
    using SimdVectorIndexed<float, float, 32, Alignment>::elements_;

    SimdVectorImpl() { }

    SimdVectorImpl(float value)
    {
        elements_ = { value, value, value, value, value, value, value, value };
    }
};

template <SimdAlignmentType Alignment>
class SimdVectorImpl<double, 32, Alignment>: public SimdVectorIndexed<double, double, 32, Alignment>
{
  public:
    using SimdVectorIndexed<double, double, 32, Alignment>::SimdVectorIndexed;
    using SimdVectorIndexed<double, double, 32, Alignment>::elements_;

    SimdVectorImpl() { }

    SimdVectorImpl(double value)
    {
        elements_ = { value, value, value, value };
    }
};

template <SimdAlignmentType Alignment>
class SimdVectorImpl<std::complex<float>, 32, Alignment>: public SimdVectorIndexed<float, std::complex<float>, 32, Alignment>
{
  public:
    using SimdVectorIndexed<float, std::complex<float>, 32, Alignment>::SimdVectorIndexed;
    using SimdVectorIndexed<float, std::complex<float>, 32, Alignment>::elements_;

    SimdVectorImpl() { }

    SimdVectorImpl(std::complex<float> value)
    {
        const float real = value.real();
        const float imag = value.imag();
        elements_ = { real, imag, real, imag, real, imag, real, imag };
    }
};

template <SimdAlignmentType Alignment>
class SimdVectorImpl<std::complex<double>, 32, Alignment>: public SimdVectorIndexed<double, std::complex<double>, 32, Alignment>
{
  public:
    using SimdVectorIndexed<double, std::complex<double>, 32, Alignment>::SimdVectorIndexed;
    using SimdVectorIndexed<double, std::complex<double>, 32, Alignment>::elements_;

    SimdVectorImpl() { }

    SimdVectorImpl(std::complex<double> value)
    {
        const double real = value.real();
        const double imag = value.imag();
        elements_ = { real, imag, real, imag };
    }
};

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 16, Alignment> real(const SimdVectorImpl<std::complex<float>, 32, Alignment> arg)
{
    return __builtin_shufflevector(arg.elements_, arg.elements_, 0, 2, 4, 6);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 16, Alignment> imag(const SimdVectorImpl<std::complex<float>, 32, Alignment> arg)
{
    return __builtin_shufflevector(arg.elements_, arg.elements_, 1, 3, 5, 7);
}

template <SimdAlignmentType Alignment>
void splitComplex(
    const SimdVectorImpl<std::complex<float>, 32, Alignment> arg0,
    const SimdVectorImpl<std::complex<float>, 32, Alignment> arg1,
    SimdVectorImpl<float, 32, Alignment>& real,
    SimdVectorImpl<float, 32, Alignment>& imag
)
{
    real = __builtin_shufflevector(arg0.elements_, arg1.elements_, 0, 2, 4, 6, 8, 10, 12, 14);
    imag = __builtin_shufflevector(arg0.elements_, arg1.elements_, 1, 3, 5, 7, 9, 11, 13, 15);
}

template <SimdAlignmentType Alignment>
void mergeComplex(
    const SimdVectorImpl<float, 32, Alignment> real,
    const SimdVectorImpl<float, 32, Alignment> imag,
    SimdVectorImpl<std::complex<float>, 32, Alignment>& arg0,
    SimdVectorImpl<std::complex<float>, 32, Alignment>& arg1
)
{
    arg0 = __builtin_shufflevector(real.elements_, imag.elements_, 0, 8, 1, 9, 2, 10, 3, 11);
    arg1 = __builtin_shufflevector(real.elements_, imag.elements_, 4, 12, 5, 13, 6, 14, 7, 15);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<std::complex<float>, 32, Alignment> flip(const SimdVectorImpl<std::complex<float>, 32, Alignment> arg)
{
    return SimdVectorImpl<std::complex<float>, 32, Alignment>(__builtin_shufflevector(arg.elements_, arg.elements_, 6, 7, 4, 5, 2, 3, 0, 1));
}

template <SimdAlignmentType Alignment>
struct AdjacentExtender<SimdVectorImpl<float, 32, Alignment>, SimdVectorImpl<float, 16, Alignment>>
{
    static SimdVectorImpl<float, 32, Alignment> extend(const SimdVectorImpl<float, 16, Alignment> arg)
    {
        return SimdVectorImpl<float, 32, Alignment>(__builtin_shufflevector(arg.elements_, arg.elements_, 0, 0, 1, 1, 2, 2, 3, 3));
    }
};

} // namespace core
} // namespace hvylya
