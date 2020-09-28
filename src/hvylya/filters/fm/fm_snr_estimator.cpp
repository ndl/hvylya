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

#include <hvylya/filters/fm/fm_snr_estimator.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename T>
FmSnrEstimator<T>::FmSnrEstimator(
    T signal_center_frequency,
    T signal_bandwidth,
    T signal_stopband,
    T noise_extractor_frequency,
    T noise_extractor_bandwidth,
    T noise_extractor_stopband,
    std::size_t update_rate,
    std::size_t snr_rate
):
    power_signal_(update_rate),
    power_noise_(update_rate),
    snr_step_(update_rate / snr_rate)
{
    T frequency_multiplier = T(), bandwidth = T();

    // For low-pass filters frequency multiplier calculation is different.
DISABLE_WARNING_PUSH("-Wfloat-equal")
    if (signal_center_frequency == T())
DISABLE_WARNING_POP()
    {
        // The integral is sqr(stopband) / 2 / stopband, which simplifies to the expression below.
        frequency_multiplier = signal_stopband / (2 * noise_extractor_frequency);
        // See comments below for derivation and note that it's only half of the
        // bandwidth in the case of the lowpass filter.
        bandwidth = (signal_bandwidth + signal_stopband) / 2;
    }
    else
    {
        // The integral is (sqr(center_frequency + bandwidth) - sqr(center_frequency - bandwidth)) / 2 / (2 * bandwidth),
        // which simplifies to the expression below.
        frequency_multiplier = signal_center_frequency / noise_extractor_frequency;
        // The actual formula for bandwidth is '2 * bandwidth + 2 * (stopband - bandwidth) / 2',
        // but after simplification it's just the sum of them.
        bandwidth = signal_bandwidth + signal_stopband;
    }

    T bandwidth_multiplier = bandwidth / (noise_extractor_bandwidth + noise_extractor_stopband);
    noise_multiplier_ = sqr(frequency_multiplier) * bandwidth_multiplier;

    Base::inputState(0).setRequiredSize(snr_step_);
    Base::inputState(1).setRequiredSize(snr_step_);
    // Turn off caching for noise data at output channel.
    Base::outputState(0).setSuggestedSize(1);
}

template <typename T>
void FmSnrEstimator<T>::reset()
{
    Base::reset();
    power_signal_.clear();
    power_noise_.clear();
}

template <typename T>
void FmSnrEstimator<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    const auto& input_data = std::get<0>(input);
    const auto& input_noise_data = std::get<1>(input);
    auto& output_data = std::get<0>(output);

    const std::size_t input_data_size =
        roundDown(
            std::min(
                std::min(input_data.size(), input_noise_data.size()),
                output_data.size() * snr_step_
            ),
            snr_step_
        );
    const std::size_t output_data_size = input_data_size / snr_step_;

    for (
        std::size_t input_index = 0, output_index = 0, remaining_snr_skip = 0;
        input_index < input_data_size;
        ++input_index, --remaining_snr_skip
    )
    {
        T input_value = input_data[input_index];
        T input_noise_value = input_noise_data[input_index];
        power_signal_.add(input_value * input_value);
        power_noise_.add(input_noise_value * input_noise_value);

        if (!remaining_snr_skip)
        {
            T power_signal = std::max(std::numeric_limits<T>::min(), power_signal_.avg());
            T power_noise = noise_multiplier_ * power_noise_.avg();
            output_data[output_index++] = 10 * std::log10(power_signal / power_noise);
            remaining_snr_skip = snr_step_;
        }
    }

    input_data.advance(input_data_size);
    input_noise_data.advance(input_data_size);
    output_data.advance(output_data_size);
}

template class hvylya::filters::FmSnrEstimator<float>;
