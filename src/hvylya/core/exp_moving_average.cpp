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

#include <hvylya/core/exp_moving_average.h>

using namespace hvylya::core;

template <typename ValueType>
ExpMovingAverage<ValueType>::ExpMovingAverage()
{
}

template <typename ValueType>
ExpMovingAverage<ValueType>::ExpMovingAverage(ScalarType decay, std::size_t init_samples)
{
    init(decay, init_samples);
}

template <typename ValueType>
void ExpMovingAverage<ValueType>::clear()
{
    init(decay_, init_samples_);
}

template <typename ValueType>
void ExpMovingAverage<ValueType>::add(ValueType value)
{
    if (initialized_)
    {
        avg_ = decay_ * avg_ + decay_complement_ * value;
    }
    else
    {
        init_sum_ += value;
        ++current_samples_;
        avg_ = init_sum_ / ScalarType(current_samples_);
        initialized_ = (current_samples_ >= init_samples_);
    }
}

template <typename ValueType>
ValueType ExpMovingAverage<ValueType>::avg() const
{
    return avg_;
}

template <typename ValueType>
void ExpMovingAverage<ValueType>::init(ScalarType decay, std::size_t init_samples)
{
    decay_ = decay;
    decay_complement_ = ScalarType(1) - decay;
    init_samples_ = init_samples;
    avg_ = ValueType(0);
    init_sum_ = ValueType(0);
    current_samples_ = 0;
    initialized_ = false;
}

template class hvylya::core::ExpMovingAverage<float>;
template class hvylya::core::ExpMovingAverage<double>;
template class hvylya::core::ExpMovingAverage<std::complex<float>>;
template class hvylya::core::ExpMovingAverage<std::complex<double>>;
