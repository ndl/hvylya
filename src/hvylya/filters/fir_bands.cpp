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

#include <hvylya/filters/fir_bands.h>

using namespace hvylya::filters;

template <typename T>
ConstFirBand<T>::ConstFirBand()
{
}

template <typename T>
ConstFirBand<T>::ConstFirBand(T min_freq, T max_freq, T target_gain, T error_weight):
    frequencies_(std::make_pair(min_freq, max_freq)),
    target_gain_(target_gain),
    error_weight_(error_weight)
{
}

template <typename T>
ConstFirBand<T>::ConstFirBand(const ConstFirBand<T>& band)
{
    *this = band;
}

template <typename T>
ConstFirBand<T>& ConstFirBand<T>::operator =(const ConstFirBand<T>& band)
{
    frequencies_ = band.frequencies_;
    target_gain_ = band.target_gain_;
    error_weight_ = band.error_weight_;
    return *this;
}

template <typename T>
std::pair<T, T> ConstFirBand<T>::frequencies() const
{
    return frequencies_;
}

template <typename T>
T ConstFirBand<T>::targetGain(T /* arg */) const
{
    return target_gain_;
}

template <typename T>
T ConstFirBand<T>::errorWeight(T /* arg */) const
{
    return error_weight_;
}

template <typename T>
void ConstFirBand<T>::setErrorWeight(T error_weight)
{
    error_weight_ = error_weight;
}

template <typename T>
DifferentiatorFirBand<T>::DifferentiatorFirBand()
{
}

template <typename T>
DifferentiatorFirBand<T>::DifferentiatorFirBand(T passband, T target_gain, T min_error_weight, T max_error_weight):
    passband_(passband),
    target_gain_(target_gain),
    min_error_weight_(min_error_weight),
    max_error_weight_(max_error_weight),
    min_error_arg_(min_error_weight / max_error_weight)
{
}

template <typename T>
std::pair<T, T> DifferentiatorFirBand<T>::frequencies() const
{
    return std::make_pair(0, passband_);
}

template <typename T>
T DifferentiatorFirBand<T>::targetGain(T arg) const
{
    return arg * passband_ * target_gain_;
}

template <typename T>
T DifferentiatorFirBand<T>::errorWeight(T arg) const
{
    arg = std::max(arg, min_error_arg_);
    return min_error_weight_ / arg;
}

template class hvylya::filters::ConstFirBand<double>;
template class hvylya::filters::ConstFirBand<long double>;

template class hvylya::filters::DifferentiatorFirBand<double>;
template class hvylya::filters::DifferentiatorFirBand<long double>;
