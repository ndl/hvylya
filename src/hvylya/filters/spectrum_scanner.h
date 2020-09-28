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
#include <hvylya/filters/fft_transformer.h>

namespace hvylya {
namespace filters {

template <class T>
class SpectrumScanner:
    public FilterGeneric<
        core::TypeList<std::complex<T>>,
        core::TypeList<>
    >
{
  public:
    typedef typename FilterBaseType<SpectrumScanner>::Type Base;

    SpectrumScanner(
        std::size_t fft_size,
        std::size_t averaging_window,
        T scale
    );

    virtual void reset() override;

    bool ready() const;

    T level(std::size_t index) const;

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override;

  private:
    std::size_t fft_size_, fft_size2_, levels_size_, averaging_window_, max_value_, current_index_;
    FftTransformer<std::complex<T>, std::complex<T>> fft_transformer_;
    core::AlignedVector<std::complex<T>> buffer_, result_;
    core::AlignedVector<std::size_t> levels_, history_;
    std::atomic<std::size_t> samples_;
    T scale_;
};

} // namespace filters
} // namespace hvylya
