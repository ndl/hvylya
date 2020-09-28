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

#include <hvylya/core/running_sum.h>
#include <hvylya/filters/filter_generic.h>

namespace hvylya {
namespace filters {

template <class T>
class FmSnrEstimator:
    public FilterGeneric<
        core::TypeList<T, T>,
        core::TypeList<T>
    >
{
  public:
    typedef typename FilterBaseType<FmSnrEstimator>::Type Base;

    FmSnrEstimator(
        T signal_center_frequency,
        T signal_bandwidth,
        T signal_stopband,
        T noise_extractor_frequency,
        T noise_extractor_bandwidth,
        T noise_extractor_stopband,
        std::size_t update_rate,
        std::size_t snr_rate
    );

    virtual void reset() override;

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override;

  private:
    core::RunningSum<T> power_signal_, power_noise_;
    std::size_t snr_step_;
    T noise_multiplier_;
};

} // namespace filters
} // namespace hvylya
