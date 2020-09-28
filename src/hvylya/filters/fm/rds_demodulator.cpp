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

#include <hvylya/filters/fm/rds_demodulator.h>

using namespace hvylya::core;
using namespace hvylya::filters;

#ifdef RDS_DUMP_DEMOD_STATS
std::size_t g_rds_demod_real = 0, g_rds_demod_imag = 0;
#endif // RDS_DUMP_DEMOD_STATS

namespace {

// 32K value seem to be enough, but use higher value just to be sure
const std::size_t AveragingWindowSize = 50000;

#ifndef ADJUST_RDS_CARRIER_PHASE
// With intermediate sampling rate of 250K and
// averaging window size above, this skip rate
// means full update of the running sum in 2 seconds.
const std::size_t SkipRate = 10;
#endif

} // anonymous namespace

template <typename T>
RdsDemodulator<T>::RdsDemodulator():
    best_phase_(0),
    skipped_samples_(0),
    best_phase_found_(false)
{
#ifdef RDS_DUMP_DEMOD_STATS
    g_rds_demod_real = g_rds_demod_imag = 0;
#endif // RDS_DUMP_DEMOD_STATS

    amplitude_levels_[0].setSize(AveragingWindowSize / ComplexVector::Elements);
    amplitude_levels_[1].setSize(AveragingWindowSize / ComplexVector::Elements);

    Base::inputState(0).setRequiredSize(ComplexVector::Elements);
    Base::inputState(1).setRequiredSize(ComplexVector::Elements);
    Base::outputState(0).setRequiredSize(ComplexVector::Elements);
}

template <typename T>
void RdsDemodulator<T>::reset()
{
    Base::reset();
    skipped_samples_ = 0;
    best_phase_found_ = false;
    amplitude_levels_[0].clear();
    amplitude_levels_[1].clear();
}

template <typename T>
void RdsDemodulator<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    const auto& input_rds_data = std::get<0>(input);
    const auto& input_carrier_data = std::get<1>(input);
    auto& output_data = std::get<0>(output);

    const std::size_t data_size = std::min(
        roundDown(output_data.size(), ComplexVector::Elements),
        std::min(
            roundDown(input_rds_data.size(), ComplexVector::Elements),
            roundDown(input_carrier_data.size(), ComplexVector::Elements)
        )
    );

    for (std::size_t index = 0; index < data_size; index += ComplexVector::Elements)
    {
        const ComplexVector demodulator = *reinterpret_cast<const ComplexVector*>(&input_carrier_data[index]);
        const ScalarHalfVector rds_values = *reinterpret_cast<const ScalarHalfVector*>(&input_rds_data[index]);

#ifdef ADJUST_RDS_CARRIER_PHASE
        *reinterpret_cast<ScalarHalfVector*>(&output_data[index]) = imag(demodulator) * rds_values;
#else
        if (best_phase_found_ && skipped_samples_ < SkipRate)
        {
            *reinterpret_cast<ScalarHalfVector*>(&output_data[index]) =
                best_phase_ == 0 ?
                real(demodulator) * rds_values :
                imag(demodulator) * rds_values;
            ++skipped_samples_;
        }
        else
        {
            const ScalarHalfVector demod_real = real(demodulator) * rds_values;
            const ScalarHalfVector demod_imag = imag(demodulator) * rds_values;

            amplitude_levels_[0].add(sum(abs(demod_real)));
            amplitude_levels_[1].add(sum(abs(demod_imag)));

            best_phase_ = amplitude_levels_[0].sum() > amplitude_levels_[1].sum() ? 0 : 1;

            if (amplitude_levels_[0].full())
            {
                best_phase_found_ = true;
            }

            *reinterpret_cast<ScalarHalfVector*>(&output_data[index]) =
                best_phase_ == 0 ?
                demod_real :
                demod_imag;
            skipped_samples_ = 0;
        }
#ifdef RDS_DUMP_DEMOD_STATS
        ++(best_phase_ == 0 ? g_rds_demod_real : g_rds_demod_imag);
#endif // RDS_DUMP_DEMOD_STATS
#endif
    }

    input_rds_data.advance(data_size);
    input_carrier_data.advance(data_size);
    output_data.advance(data_size);
}

template class hvylya::filters::RdsDemodulator<float>;
