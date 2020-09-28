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

#include <hvylya/filters/ifir_band.h>

namespace hvylya {
namespace filters {

template <typename T>
class ConstFirBand: public IFirBand<T>
{
  public:
    ConstFirBand();

    ConstFirBand(const ConstFirBand<T>& band);

    ConstFirBand(T min_freq, T max_freq, T target_gain, T error_weight);

    ConstFirBand<T>& operator =(const ConstFirBand<T>& band);

    virtual std::pair<T, T> frequencies() const override;

    virtual T targetGain(T arg = 0) const override;

    virtual T errorWeight(T arg = 0) const override;

    void setErrorWeight(T error_weight);

  private:
    std::pair<T, T> frequencies_;
    T target_gain_, error_weight_;
};

template <typename T>
class DifferentiatorFirBand: public IFirBand<T>
{
  public:
    DifferentiatorFirBand();

    DifferentiatorFirBand(T passband, T target_gain, T min_error_weight, T max_error_weight);

    virtual std::pair<T, T> frequencies() const override;

    virtual T targetGain(T arg = 0) const override;

    virtual T errorWeight(T arg = 0) const override;

  private:
    T passband_, target_gain_, min_error_weight_, max_error_weight_, min_error_arg_;
};

} // namespace hvylya
} // namespace filters
