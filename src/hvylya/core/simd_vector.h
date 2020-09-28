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

#include <immintrin.h>

namespace hvylya {
namespace core {

enum SimdAlignmentType
{
    NonAligned = 0,
    Aligned = 1
};

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
class SimdVectorImpl { };

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
struct SimdVectorTraits { };

template <typename ParamScalarType, typename ParamElementType, std::size_t ParamByteSize, SimdAlignmentType ParamAlignment>
class SimdVectorBase
{
  public:
    static const std::size_t ByteSize = ParamByteSize;
    static const std::size_t Elements = ParamByteSize / sizeof(ParamElementType);
    static const std::size_t Scalars = ParamByteSize / sizeof(ParamScalarType);
    static const std::size_t ScalarsInElement = sizeof(ParamElementType) / sizeof(ParamScalarType);
    static const std::size_t Alignment = (ParamAlignment == Aligned) ? ParamByteSize : 1;

    typedef ParamScalarType ScalarType;
    typedef ParamElementType ElementType;
    typedef typename SimdVectorTraits<ParamScalarType, ParamByteSize, ParamAlignment>::VectorType VectorType;
    typedef SimdVectorImpl<ParamScalarType, ParamByteSize, ParamAlignment> FlattenedType;
    typedef SimdVectorImpl<ParamElementType, ParamByteSize / 2, ParamAlignment> HalfVectorType;

    VectorType elements_;

    SimdVectorBase() { }

    SimdVectorBase(VectorType elements): elements_(elements) { }
};

template <typename ScalarType, typename ElementType, std::size_t ByteSize, SimdAlignmentType Alignment>
class SimdVectorIndexed: public SimdVectorBase<ScalarType, ElementType, ByteSize, Alignment>
{
  public:
    using SimdVectorBase<ScalarType, ElementType, ByteSize, Alignment>::SimdVectorBase;
    using SimdVectorBase<ScalarType, ElementType, ByteSize, Alignment>::elements_;

    ScalarType operator [](const std::size_t index) const
    {
        return elements_[index];
    }
};

template <typename ScalarType, std::size_t ByteSize, SimdAlignmentType Alignment>
class SimdVectorIndexed<ScalarType, std::complex<ScalarType>, ByteSize, Alignment>:
    public SimdVectorBase<ScalarType, std::complex<ScalarType>, ByteSize, Alignment>
{
  public:
    using SimdVectorBase<ScalarType, std::complex<ScalarType>, ByteSize, Alignment>::SimdVectorBase;
    using SimdVectorBase<ScalarType, std::complex<ScalarType>, ByteSize, Alignment>::elements_;

    std::complex<ScalarType> operator [](const std::size_t index) const
    {
        return std::complex<ScalarType>(elements_[2 * index], elements_[2 * index + 1]);
    }
};

} // namespace core
} // namespace hvylya

#include <hvylya/core/simd_vector_common.h>

// Implementations for particular sizes and instruction set-specific functions.

#ifdef __SSSE3__
#   include <hvylya/core/simd_vector_ssse3.h>
#endif

#ifdef __AVX2__
#   include <hvylya/core/simd_vector_avx2.h>
#endif

namespace hvylya {
namespace core {

// Default vector size for this platform.
enum: std::size_t
{
    MaxSimdByteSize =
#if defined(__AVX2__)
    32,
#elif defined(__SSSE3__)
    16,
#else
#   error "Unsupported SIMD platform"
#endif
    DefaultScalarByteSize = 16
};

// Wrapper over SimdVectorImpl that sets the size to the one available for this platform.
template <typename T, SimdAlignmentType Alignment>
class SimdVector: public SimdVectorImpl<T, MaxSimdByteSize, Alignment>
{
  public:
    using SimdVectorImpl<T, MaxSimdByteSize, Alignment>::SimdVectorImpl;

    SimdVector()
    {
    }

    SimdVector(const SimdVectorImpl<T, MaxSimdByteSize, Alignment>& other):
        SimdVectorImpl<T, MaxSimdByteSize, Alignment>(other.elements_)
    {
    }
};

// Generic implementation of AdjacentExtender for SimdVector, as it's not matched by specializations for
// particular SimdVectorImpl's.
template <typename T, SimdAlignmentType Alignment, typename InputVector>
struct AdjacentExtender<SimdVector<T, Alignment>, InputVector>
{
    static SimdVector<T, Alignment> extend(const InputVector arg)
    {
        return SimdVector<T, Alignment>(AdjacentExtender<SimdVectorImpl<T, SimdVector<T, Alignment>::ByteSize, Alignment>, InputVector>::extend(arg).elements_);
    }
};

// Wrapper over SimdVectorImpl that is used for single real number representation.
template <typename T, std::size_t ByteSize = DefaultScalarByteSize, SimdAlignmentType Alignment = Aligned>
class SimdScalar: public SimdVectorImpl<T, ByteSize, Alignment>
{
  public:
    using SimdVectorImpl<T, ByteSize, Alignment>::SimdVectorImpl;

    SimdScalar()
    {
    }

    SimdScalar(const SimdVectorImpl<T, ByteSize, Alignment>& other):
        SimdVectorImpl<T, ByteSize, Alignment>(other.elements_)
    {
    }
};

// Wrapper over SimdVectorImpl that is used for single complex number representation.
template <typename T, std::size_t ByteSize = DefaultScalarByteSize, SimdAlignmentType Alignment = Aligned>
class SimdComplex: public SimdVectorImpl<std::complex<T>, ByteSize, Alignment>
{
  public:
    using SimdVectorImpl<std::complex<T>, ByteSize, Alignment>::SimdVectorImpl;

    SimdComplex()
    {
    }

    SimdComplex(const SimdVectorImpl<std::complex<T>, ByteSize, Alignment>& other):
        SimdVectorImpl<std::complex<T>, ByteSize, Alignment>(other.elements_)
    {
    }
};

template <typename T>
bool isPointerAligned(const T* ptr)
{
    std::size_t ptr_val = reinterpret_cast<std::size_t>(ptr);
    return !(ptr_val & (MaxSimdByteSize - 1));
}

template <typename T>
void alignPointer(const T*& ptr, std::size_t& offset)
{
    std::size_t ptr_val = reinterpret_cast<std::size_t>(ptr);
    offset = (ptr_val & (MaxSimdByteSize - 1)) / sizeof(T);
    ptr = reinterpret_cast<T*>(ptr_val & ~(MaxSimdByteSize - 1));
}

} // namespace core
} // namespace hvylya
