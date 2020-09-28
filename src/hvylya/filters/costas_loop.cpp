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

#include <hvylya/filters/costas_loop.h>

#include <hvylya/filters/iir_filters_designer.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename T>
CostasLoop<T>::CostasLoop(T lowpass_freq, T phase_error_gain):
    phase_error_gain_(phase_error_gain),
    rotation_(T(1)),
    branch_real_filter_(IirFiltersDesigner<T>::createLowpassFirstOrderFilter(lowpass_freq)),
    branch_imag_filter_(IirFiltersDesigner<T>::createLowpassFirstOrderFilter(lowpass_freq))
{
}

template <typename T>
void CostasLoop<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    const auto& input_signal = std::get<0>(input);
    const auto& input_pilot = std::get<1>(input);
    auto& output_data = std::get<0>(output);

    const std::size_t data_size = std::min(std::min(input_signal.size(), input_pilot.size()), output_data.size());

    for (std::size_t index = 0; index < data_size; ++index)
    {
        std::complex<T> rotated = input_pilot[index] * rotation_;
        output_data[index] = rotated;
        T phase_error = branch_real_filter_.filter(rotated.real() * input_signal[index]) * branch_imag_filter_.filter(-rotated.imag() * input_signal[index]);
        rotation_ *= std::exp(std::complex<T>(0, -phase_error * phase_error_gain_));
        rotation_ /= std::abs(rotation_);
    }

    input_signal.advance(data_size);
    input_pilot.advance(data_size);
    output_data.advance(data_size);
}

template class hvylya::filters::CostasLoop<float>;
