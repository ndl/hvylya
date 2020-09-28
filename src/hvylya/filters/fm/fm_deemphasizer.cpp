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

#include <hvylya/filters/fm/fm_deemphasizer.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename T>
FmDeemphasizer<T>::FmDeemphasizer(uint32_t sample_rate, T tau):
    sample_rate_(sample_rate),
    tau_(tau),
    last_demod_value_(T()),
    last_deemph_value_(T())
{
    CHECK_NE(sample_rate, 0) << "sample_rate cannot be zero";
    CHECK_GT(tau, 0) << "tau must be > 0";
}

template <typename T>
void FmDeemphasizer<T>::reset()
{
    Base::reset();
    last_demod_value_ = T();
    last_deemph_value_ = T();
}

template <typename T>
void FmDeemphasizer<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    const auto& input_data = std::get<0>(input);
    auto& output_data = std::get<0>(output);

    const std::size_t data_size = std::min(output_data.size(), input_data.size());

    const T w_p = T(1.0 / tau_);
    const T w_pp = std::tan(w_p / (2 * sample_rate_));
    const T fb = T((w_pp - 1.0) / (w_pp + 1.0));
    const T ff = T(w_pp / (1.0 + w_pp));

    T last_demod_value = last_demod_value_;
    T last_deemph_value = last_deemph_value_;

    for (std::size_t input_index = 0; input_index < data_size; ++input_index)
    {
        T demod_value = input_data[input_index];
        T deemph_value = ff * (last_demod_value + demod_value) - fb * last_deemph_value;
        output_data[input_index] = deemph_value;
        last_demod_value = demod_value;
        last_deemph_value = deemph_value;
    }

    last_deemph_value_ = last_deemph_value;
    last_demod_value_ = last_demod_value;

    input_data.advance(data_size);
    output_data.advance(data_size);
}

template class hvylya::filters::FmDeemphasizer<float>;
