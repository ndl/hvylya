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
#include <hvylya/filters/iir_filter.h>

namespace hvylya {
namespace filters {

template <class T>
class CostasLoop:
    public FilterGeneric<
        core::TypeList<T, std::complex<T>>,
        core::TypeList<std::complex<T>>
    >
{
  public:
    typedef typename FilterBaseType<CostasLoop>::Type Base;

    CostasLoop(T lowpass_freq, T phase_error_gain);

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override;

  private:
    T phase_error_gain_;
    std::complex<T> rotation_;
    IirFilter<T, T, 2, 2> branch_real_filter_, branch_imag_filter_;
};

} // namespace filters
} // namespace hvylya
