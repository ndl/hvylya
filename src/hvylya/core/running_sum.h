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

namespace hvylya {
namespace core {

template <typename ValueType>
class RunningSum
{
  public:
    typedef typename NumericTypeMapper<ValueType>::ScalarType ScalarType;

    RunningSum();

    explicit RunningSum(std::size_t max_size);

    void setSize(std::size_t max_size);

    void clear();

    void add(ValueType value);

    bool empty() const;

    bool full() const;

    std::size_t size() const;

    ValueType sum() const;

    ValueType avg() const;

  private:
    ValueType sum_, compensation_;
    std::size_t index_, size_, iterations_, max_iterations_;
    std::vector<ValueType> data_;

    void init();

    void addImpl(ValueType value);
};

} // namespace core
} // namespace hvylya
