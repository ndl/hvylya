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

namespace hvylya {
namespace core {

template <class T>
struct Slice
{
  public:
    Slice(T* data, std::size_t size):
        data_(data),
        size_(size),
        advanced_size_(0)
    {
    }

    Slice(std::vector<T>& vec):
        data_(&vec[0]),
        size_(vec.size()),
        advanced_size_(0)
    {
    }

    Slice(AlignedVector<T>& vec):
        data_(&vec[0]),
        size_(vec.size()),
        advanced_size_(0)
    {
    }

    const T& operator [](std::size_t index) const { return data_[index]; }

    T& operator [](std::size_t index) { return data_[index]; }

    const T* begin() const { return &data_[0]; }

    T* begin() { return &data_[0]; }

    const T* end() const { return &data_[size_]; }

    T* end() { return &data_[size_]; }

    std::size_t size() const { return size_; }

    void advance(std::size_t advance_size) const { advanced_size_ += advance_size; CHECK_LE(advanced_size_, size_); }

    std::size_t advancedSize() const { return advanced_size_; }

  private:
    T* data_;
    std::size_t size_;
    mutable std::size_t advanced_size_;
};

} // namespace common
} // namespace hvylya
