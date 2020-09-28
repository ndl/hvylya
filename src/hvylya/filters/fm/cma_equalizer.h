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

//#define DUMP_CMA_EQUALIZER_STATS

#ifdef DUMP_CMA_EQUALIZER_STATS
extern std::size_t g_samples, g_taps_updates, g_gain_updates;
#endif // DUMP_CMA_EQUALIZER_STATS

namespace hvylya {
namespace filters {

template <typename T>
class FmCmaEqualizer:
    public FilterGeneric<
        core::TypeList<std::complex<T>>,
        core::TypeList<std::complex<T>>
    >
{
  public:
    typedef typename FilterBaseType<FmCmaEqualizer>::Type Base;

    FmCmaEqualizer(std::size_t taps_count);

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override;

  private:
    typedef std::complex<T> ValueType;
    typedef double AccScalarType;
    typedef std::complex<AccScalarType> AccType;
    typedef core::SimdVector<ValueType, core::Aligned> ComplexVector;
    typedef core::SimdVector<ValueType, core::NonAligned> ComplexVectorNonAligned;
    typedef core::SimdVector<T, core::Aligned> ScalarVector;
    typedef core::SimdVector<AccType, core::Aligned> AccComplexVector;
    typedef core::SimdVector<AccScalarType, core::Aligned> AccScalarVector;

    core::AlignedVector<ValueType> taps_, outputs_;
    std::vector<std::size_t> taps_indices_;
    std::vector<std::int8_t> taps_enabled_;
    core::AlignedVector<ValueType> adjusted_taps_;
    std::vector<AccScalarType> decays_, weight_updates_;
    core::AlignedVector<AccType> a_, b_;
    core::AlignedVector<ValueType> a_interm_, b_interm_;
    std::size_t taps_count_, taps_count_mask_, taps_update_freq_, gain_update_freq_, accum_update_freq_, left_until_taps_updates_,
        left_until_gain_updates_, left_until_disabled_taps_updates_, left_until_accum_updates_, taps_enabled_count_,
        accumulated_samples_, cur_output_index_;
    ValueType gain_;
    AccScalarType cur_weight_;
    T norm_weight_, tap0_mag2_, tap_reg_, last_taps_diff_;
    bool min_samples_accumulated_;

    ValueType getOutput(ValueType sample);

    void updateIntermAccumulators(ValueType sample, ValueType new_output);

    void updateAccumulators();

    void updateNormWeight();

    void updateGain();

    T updateTaps(bool calc_disabled_taps);

    void updateTapReg(T tap_mag_sum);

    ValueType getCorrelation(std::size_t row, std::size_t col);
};

} // namespace filters
} // namespace hvylya
