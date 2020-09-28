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
struct SimdVectorTraits<float, 8, Aligned>
{
  typedef float __attribute__((vector_size(8), aligned(8))) VectorType;
};

template <>
struct SimdVectorTraits<float, 8, NonAligned>
{
  typedef float __attribute__((vector_size(8), aligned(1))) VectorType;
};

template <SimdAlignmentType Alignment>
class SimdVectorImpl<float, 8, Alignment>: public SimdVectorIndexed<float, float, 8, Alignment>
{
  public:
    using SimdVectorIndexed<float, float, 8, Alignment>::SimdVectorIndexed;
    using SimdVectorIndexed<float, float, 8, Alignment>::elements_;

    SimdVectorImpl() { }

    SimdVectorImpl(float value)
    {
        elements_ = { value, value };
    }
};

template <SimdAlignmentType Alignment>
class SimdVectorImpl<std::complex<float>, 8, Alignment>: public SimdVectorIndexed<float, std::complex<float>, 8, Alignment>
{
  public:
    using SimdVectorIndexed<float, std::complex<float>, 8, Alignment>::SimdVectorIndexed;
    using SimdVectorIndexed<float, std::complex<float>, 8, Alignment>::elements_;

    SimdVectorImpl() { }

    SimdVectorImpl(std::complex<float> value)
    {
        elements_ = { value.real(), value.imag() };
    }
};

template <SimdAlignmentType Alignment>
float sum(const SimdVectorImpl<float, 8, Alignment> arg)
{
    return arg[0] + arg[1];
}

template <SimdAlignmentType Alignment>
std::complex<float> sum(const SimdVectorImpl<std::complex<float>, 8, Alignment> arg)
{
        return std::complex<float>(arg[0], arg[1]);
}

template <SimdAlignmentType Alignment>
SimdVectorImpl<float, 8, Alignment> abs(const SimdVectorImpl<float, 8, Alignment> arg)
{
    SimdVectorImpl<float, 8, Alignment> result;
    result.elements_ = { std::abs(arg[0]), std::abs(arg[1]) };
    return result;
}

} // namespace core
} // namespace hvylya
