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

#include <hvylya/core/simd_vector.h>

namespace hvylya {
namespace core {

template <class T, std::size_t Alignment = MaxSimdByteSize>
struct AlignedVector: NonCopyable
{
  public:
    enum: std::size_t
    {
        AlignmentInBytes = Alignment,
        AlignmentInElements = Alignment / sizeof(T)
    };

    AlignedVector():
        size_(0),
        capacity_(0),
        data_(nullptr, &free)
    {
    }

    explicit AlignedVector(std::size_t size, T fill_value = T()):
        size_(size),
        capacity_(roundUpToPowerOfTwo(size_)),
        data_(static_cast<T*>(aligned_alloc(Alignment, core::roundUp(capacity_ * sizeof(T), Alignment))), &free)
    {
        std::fill(&data_.get()[0], &data_.get()[size_], fill_value);
    }

    AlignedVector(AlignedVector&& vec):
        size_(vec.size_),
        capacity_(vec.capacity_),
        data_(std::move(vec.data_))
    {
    }

    ~AlignedVector() { }

    const T& operator [](std::size_t index) const { return data_.get()[index]; }

    T& operator [](std::size_t index) { return data_.get()[index]; }

    const T* begin() const { return &data_.get()[0]; }

    T* begin() { return &data_.get()[0]; }

    const T* end() const { return &data_.get()[size_]; }

    T* end() { return &data_.get()[size_]; }

    bool empty() const { return !size_; }

    std::size_t size() const { return size_; }

    void resize(std::size_t new_size, T fill_value = T())
    {
        if (new_size > capacity_)
        {
            capacity_ = roundUpToPowerOfTwo(new_size);
            DataPtr new_data(static_cast<T*>(aligned_alloc(Alignment, core::roundUp(capacity_ * sizeof(T), Alignment))), &free);
            if (data_)
            {
                std::copy(&data_.get()[0], &data_.get()[size_], &new_data.get()[0]);
            }
            data_ = std::move(new_data);
        }

        if (new_size > size_)
        {
            std::fill(&data_.get()[size_], &data_.get()[new_size], fill_value);
        }

        size_ = new_size;
    }

    void clear()
    {
        size_ = 0;
    }

  private:
    typedef std::unique_ptr<T, void (*)(void*)> DataPtr;
    std::size_t size_, capacity_;
    DataPtr data_;
};

} // namespace common
} // namespace hvylya
