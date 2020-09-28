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

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator +(const SimdVectorImpl<T, ByteSize, Alignment> arg0, const SimdVectorImpl<T, ByteSize, Alignment> arg1)
{
    return SimdVectorImpl<T, ByteSize, Alignment>(arg0.elements_ + arg1.elements_);
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator +(const SimdVectorImpl<T, ByteSize, Alignment> arg0, const T arg1)
{
    return arg0 + SimdVectorImpl<T, ByteSize, Alignment>(arg1);
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator +(const T arg0, const SimdVectorImpl<T, ByteSize, Alignment> arg1)
{
    return SimdVectorImpl<T, ByteSize, Alignment>(arg0) + arg1;
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment>& operator +=(SimdVectorImpl<T, ByteSize, Alignment>& arg0, const SimdVectorImpl<T, ByteSize, Alignment> arg1)
{
    arg0.elements_ += arg1.elements_;
    return arg0;
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment>& operator +=(SimdVectorImpl<T, ByteSize, Alignment>& arg0, const T arg1)
{
    arg0 += SimdVectorImpl<T, ByteSize, Alignment>(arg1);
    return arg0;
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator -(const SimdVectorImpl<T, ByteSize, Alignment> arg0, const SimdVectorImpl<T, ByteSize, Alignment> arg1)
{
    return SimdVectorImpl<T, ByteSize, Alignment>(arg0.elements_ - arg1.elements_);
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator -(const SimdVectorImpl<T, ByteSize, Alignment> arg0, const T arg1)
{
    return arg0 - SimdVectorImpl<T, ByteSize, Alignment>(arg1);
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator -(const T arg0, const SimdVectorImpl<T, ByteSize, Alignment> arg1)
{
    return SimdVectorImpl<T, ByteSize, Alignment>(arg0) - arg1;
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator -(const SimdVectorImpl<T, ByteSize, Alignment> arg)
{
    return SimdVectorImpl<T, ByteSize, Alignment>(-arg.elements_);
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment>& operator -=(SimdVectorImpl<T, ByteSize, Alignment>& arg0, const SimdVectorImpl<T, ByteSize, Alignment> arg1)
{
    arg0.elements_ -= arg1.elements_;
    return arg0;
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment>& operator -=(SimdVectorImpl<T, ByteSize, Alignment>& arg0, const T arg1)
{
    arg0 -= SimdVectorImpl<T, ByteSize, Alignment>(arg1);
    return arg0;
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator *(const SimdVectorImpl<T, ByteSize, Alignment> arg0, const SimdVectorImpl<T, ByteSize, Alignment> arg1)
{
    return SimdVectorImpl<T, ByteSize, Alignment>(arg0.elements_ * arg1.elements_);
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment>& operator *=(SimdVectorImpl<T, ByteSize, Alignment>& arg0, const SimdVectorImpl<T, ByteSize, Alignment> arg1)
{
    // Note: we're not multiplying elements_ direcly, but calling the "proper" multiplication method
    // to make sure it works correctly for complex values.
    arg0 = arg0 * arg1;
    return arg0;
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator *(const SimdVectorImpl<T, ByteSize, Alignment> arg0, const typename SimdVectorImpl<T, ByteSize, Alignment>::FlattenedType arg1)
{
    return SimdVectorImpl<T, ByteSize, Alignment>(arg0.elements_ * arg1.elements_);
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator *(const typename SimdVectorImpl<T, ByteSize, Alignment>::FlattenedType arg0, const SimdVectorImpl<T, ByteSize, Alignment> arg1)
{
    return SimdVectorImpl<T, ByteSize, Alignment>(arg0.elements_ * arg1.elements_);
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment>& operator *=(SimdVectorImpl<T, ByteSize, Alignment>& arg0, const typename SimdVectorImpl<T, ByteSize, Alignment>::FlattenedType arg1)
{
    // Here we're multiplying elements_ directly, as that's the correct behavior both for normal and complex values.
    arg0.elements_ *= arg1.elements_;
    return arg0;
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator *(const SimdVectorImpl<T, ByteSize, Alignment> arg0, const T arg1)
{
    return arg0 * SimdVectorImpl<T, ByteSize, Alignment>(arg1);
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator *(const T arg0, const SimdVectorImpl<T, ByteSize, Alignment> arg1)
{
    return SimdVectorImpl<T, ByteSize, Alignment>(arg0) * arg1;
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator /(const SimdVectorImpl<T, ByteSize, Alignment> arg0, const typename SimdVectorImpl<T, ByteSize, Alignment>::FlattenedType arg1)
{
    return SimdVectorImpl<T, ByteSize, Alignment>(arg0.elements_ / arg1.elements_);
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment>& operator /=(SimdVectorImpl<T, ByteSize, Alignment>& arg0, const typename SimdVectorImpl<T, ByteSize, Alignment>::FlattenedType arg1)
{
    // Here we're dividing elements_ directly, as that's the correct behavior both for normal and complex values.
    arg0.elements_ /= arg1.elements_;
    return arg0;
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> operator /(const SimdVectorImpl<T, ByteSize, Alignment> arg0, const T arg1)
{
    return arg0 / SimdVectorImpl<T, ByteSize, Alignment>(arg1);
}

template <typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
SimdVectorImpl<T, ByteSize, Alignment> fusedMultiplyAdd(const SimdVectorImpl<T, ByteSize, Alignment> arg0, const SimdVectorImpl<T, ByteSize, Alignment> arg1, const SimdVectorImpl<T, ByteSize, Alignment> arg2)
{
    return arg0 * arg1 + arg2;
}

// Empty implementation in general case, will cause errors if used.
template <typename ExtendedVector, typename InputVector>
struct AdjacentExtender
{
};

// Identity implementation.
template <typename InputVector>
struct AdjacentExtender<InputVector, InputVector>
{
    static InputVector extend(const InputVector arg)
    {
        return arg;
    }
};

template <typename ExtendedVector, typename InputVector>
ExtendedVector extendAdjacentAs(const InputVector arg)
{
    return AdjacentExtender<ExtendedVector, InputVector>::extend(arg);
}

// Identity implementation.
template <typename DstVectorType, typename T, std::size_t ByteSize, SimdAlignmentType Alignment>
std::array<DstVectorType, 1> castVectorImpl(SimdVectorImpl<T, ByteSize, Alignment> arg, SimdVectorImpl<T, ByteSize, Alignment> /*unused*/)
{
    return std::array<DstVectorType, 1>(DstVectorType(arg.elements_));
}

template <typename DstVectorType, typename SrcVectorType>
std::array<DstVectorType, SrcVectorType::Elements / DstVectorType::Elements> castVector(SrcVectorType arg)
{
    static_assert(
        (SrcVectorType::Elements / DstVectorType::Elements) * DstVectorType::Elements == SrcVectorType::Elements,
        "Unsupported SrcVectorType and DstVectorType combination!"
    );
    return castVectorImpl<DstVectorType>(typename SrcVectorType::FlattenedType(arg.elements_), typename DstVectorType::FlattenedType());
}

} // namespace core
} // namespace hvylya
