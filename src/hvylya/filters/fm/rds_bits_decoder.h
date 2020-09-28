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
#include <hvylya/core/running_sum.h>

// #define RDS_DUMP_BITS
// #define RDS_DUMP_CLOCK_STATS

#ifdef RDS_DUMP_CLOCK_STATS
extern std::size_t g_clocks_stats[16 * 4];
#endif // RDS_DUMP_CLOCK_STATS

namespace hvylya {
namespace filters {

using namespace hvylya::core;

template <class T>
class RdsBitsDecoder:
    public FilterGeneric<
        TypeList<T, std::complex<T>>,
        TypeList<
            std::int8_t
#ifdef RDS_DUMP_BITS
            , T
#endif
        >
    >
{
  public:
    typedef typename FilterBaseType<RdsBitsDecoder>::Type Base;

    RdsBitsDecoder();

    virtual void reset() override;

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override;

  private:
    enum: std::size_t
    {
        // 19000 (pilot frequency) / 1187.5 (symbol rate) * 4 (number of zero crossings
        // for I / Q per single pilot period).
        FrequencyDivider = 16 * 4
    };

    RunningSum<T> mags_sums_[FrequencyDivider];
    std::complex<T> prev_pll_value_;
    T prev_rds_value_;
    int prev_sign_real_, prev_sign_imag_, prev_symbol_;
    std::size_t clock_counter_, clocks_passed_, best_clock_;

    std::size_t selectBestClock() const;
};

} // namespace filters
} // namespace hvylya
