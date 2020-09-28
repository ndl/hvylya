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

#include <hvylya/filters/filter_generic.h>

namespace hvylya {
namespace filters {

template <class T>
class FmPllDecoder:
    public FilterGeneric<
        core::TypeList<std::complex<T>>,
        core::TypeList<T>
    >
{
  public:
    typedef typename FilterBaseType<FmPllDecoder>::Type Base;

    FmPllDecoder(std::size_t sample_rate);

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override;

  private:
    typedef core::SimdVector<std::complex<T>, core::Aligned> ComplexVector;
    typedef core::SimdVector<T, core::Aligned> ScalarVector;

    T phase_, phase_gain_, fm_gain_;

    T getClampedByPi(T value);
};

} // namespace filters
} // namespace hvylya
