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

#include <hvylya/core/running_sum.h>

using namespace hvylya::core;

template <typename ValueType>
RunningSum<ValueType>::RunningSum()
{
    init();
}

template <typename ValueType>
RunningSum<ValueType>::RunningSum(std::size_t max_size)
{
    init();
    setSize(max_size);
}

template <typename ValueType>
void RunningSum<ValueType>::setSize(std::size_t max_size)
{
    CHECK_NE(max_size, 0) << "max size for the running sum should be > 0";
    data_.resize(max_size);
}

template <typename ValueType>
void RunningSum<ValueType>::clear()
{
    init();
    std::fill(data_.begin(), data_.end(), ValueType());
}

template <typename ValueType>
void RunningSum<ValueType>::add(ValueType value)
{
    if (size_ == data_.size())
    {
        addImpl(-data_[index_]);
    }
    else
    {
        ++size_;
    }

    addImpl(value);
    data_[index_] = value;
    index_ = (index_ + 1) % data_.size();

    ++iterations_;
    if (iterations_ > max_iterations_)
    {
        iterations_ = 0;
        sum_ = ValueType();
        compensation_ = 0;
        for (auto v: data_)
        {
            addImpl(v);
        }
    }
}

template <typename ValueType>
bool RunningSum<ValueType>::empty() const
{
    return !size_;
}

template <typename ValueType>
bool RunningSum<ValueType>::full() const
{
    return size_ == data_.size();
}

template <typename ValueType>
std::size_t RunningSum<ValueType>::size() const
{
    return size_;
}

template <typename ValueType>
ValueType RunningSum<ValueType>::sum() const
{
    return sum_;
}

template <typename ValueType>
ValueType RunningSum<ValueType>::avg() const
{
    CHECK(size_) << "attempted to take average from the running sum containing zero elements";
    return sum_ / ScalarType(size_);
}

template <typename ValueType>
void RunningSum<ValueType>::init()
{
    sum_ = ValueType();
    compensation_ = 0;
    index_ = 0;
    size_ = 0;
    iterations_ = 0;
    ScalarType eps_inv = 1 / std::numeric_limits<ScalarType>::epsilon();
    max_iterations_ =
        eps_inv < ScalarType(std::numeric_limits<std::size_t>::max()) ?
        std::size_t(eps_inv) :
        std::numeric_limits<std::size_t>::max();
}

template <typename ValueType>
void RunningSum<ValueType>::addImpl(ValueType value)
{
    // Using Kahan's summation algorithm. Shewchuk's exact summation
    // would be more elegant (= no need to re-sum and better accuracy),
    // but also significantly more expensive, so doesn't seem to be
    // worth it for our use cases.
    ValueType corrected_value = value - compensation_;
    ValueType new_sum = sum_ + corrected_value;
    compensation_ = (new_sum - sum_) - corrected_value;
    sum_ = new_sum;
}

template class hvylya::core::RunningSum<float>;
template class hvylya::core::RunningSum<double>;
template class hvylya::core::RunningSum<std::complex<float>>;
template class hvylya::core::RunningSum<std::complex<double>>;
