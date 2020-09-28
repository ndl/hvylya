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

//#define ADJUST_RDS_CARRIER_PHASE
//#define RDS_DUMP_DEMOD_STATS

#ifdef RDS_DUMP_DEMOD_STATS
extern std::size_t g_rds_demod_real, g_rds_demod_imag;
#endif // RDS_DUMP_DEMOD_STATS

namespace hvylya {
namespace filters {

using namespace hvylya::core;

template <class T>
class RdsDemodulator:
    public FilterGeneric<
        TypeList<T, std::complex<T>>,
        TypeList<T>
    >
{
  public:
    typedef typename FilterBaseType<RdsDemodulator>::Type Base;

    RdsDemodulator();

    virtual void reset() override;

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override;

  private:
    typedef core::SimdVector<std::complex<T>, core::NonAligned> ComplexVector;
    typedef typename core::SimdVector<T, core::NonAligned>::HalfVectorType ScalarHalfVector;

    RunningSum<T> amplitude_levels_[2];
    std::size_t best_phase_, skipped_samples_;
    bool best_phase_found_;
};

} // namespace filters
} // namespace hvylya
