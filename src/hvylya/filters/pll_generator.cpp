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

#include <hvylya/filters/pll_generator.h>

#include <hvylya/core/approx_trigonometry.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename T>
PllGenerator<T>::PllGenerator(T loop_bandwidth, T min_frequency, T max_frequency):
    loop_bandwidth_(loop_bandwidth),
    min_frequency_(min_frequency),
    max_frequency_(max_frequency),
    damping_(T(std::sqrt(2.0) / 2.0)),
    phase_(0),
    frequency_(T((min_frequency + max_frequency) / 2.0))
{
    CHECK_GT(loop_bandwidth, 0) << "loop bandwidth should be > 0";
    CHECK_GE(min_frequency, 0) << "min_frequency should be >= 0";
    CHECK_LE(min_frequency, max_frequency) << "min_frequency should be <= max_frequency";

    T denom = T(1.0 + 2.0 * damping_ * loop_bandwidth_ + loop_bandwidth_ * loop_bandwidth_);
    alpha_ = T(4.0 * damping_ * loop_bandwidth_ / denom);
    beta_ = T(4.0 * loop_bandwidth_ * loop_bandwidth_ / denom);
    Base::inputState(0).setRequiredSize(2 * ComplexVector::Elements);
    Base::outputState(0).setRequiredSize(2 * ComplexVector::Elements);
}

template <typename T>
void PllGenerator<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    typedef ApproxTrigonometrySimdFromVector<ComplexVector> ApproxTrig;

    const auto& input_data = std::get<0>(input);
    auto& output_data = std::get<0>(output);

    CHECK(isPointerAligned(&input_data[0]));
    CHECK(isPointerAligned(&output_data[0]));

    const std::size_t data_size =
        std::min(
            roundDown(input_data.size(), 2 * ComplexVector::Elements),
            roundDown(output_data.size(), 2 * ComplexVector::Elements)
        );

    for (std::size_t input_index = 0; input_index < data_size; input_index += 2 * ComplexVector::Elements)
    {
        ScalarVector sample_phase = ApproxTrig::atan2(
            *reinterpret_cast<const ComplexVector*>(&input_data[input_index]),
            *reinterpret_cast<const ComplexVector*>(&input_data[input_index + ComplexVector::Elements])
        );

        T phases[ScalarVector::Elements];

        for (std::size_t i = 0; i < ScalarVector::Elements; ++i)
        {
            phases[i] = phase_;

            T phase_error = getClampedByPi(sample_phase[i] - phase_);

            frequency_ = frequency_ + beta_ * phase_error;
            phase_ = phase_ + frequency_ + alpha_ * phase_error;

            phase_ = getClampedByPi(phase_);
            clampFrequency();
        }

        mergeComplex(
            ApproxTrig::cos(ScalarVector(*reinterpret_cast<const ScalarVector*>(phases))),
            ApproxTrig::sin(ScalarVector(*reinterpret_cast<const ScalarVector*>(phases))),
            *reinterpret_cast<ComplexVector*>(&output_data[input_index]),
            *reinterpret_cast<ComplexVector*>(&output_data[input_index + ComplexVector::Elements])
        );
    }

    input_data.advance(data_size);
    output_data.advance(data_size);
}

template <typename T>
T PllGenerator<T>::getClampedByPi(T value)
{
    // Pure arithmetic version of it seems to be slower
    // return value - 2 * M_PI * floor(value * (1.0 / (2 * M_PI))) - M_PI;
    if (value > M_PI)
    {
        do
        {
            value -= T(2 * M_PI);
        }
        while (value > M_PI);
    }
    else
    {
        while (value < -M_PI)
        {
            value += T(2 * M_PI);
        }
    }
    return value;
}

template <typename T>
void PllGenerator<T>::clampFrequency()
{
    if (frequency_ > max_frequency_)
    { 
        frequency_ = max_frequency_;
    }
    else if (frequency_ < min_frequency_)
    {
        frequency_ = min_frequency_;
    }
}

template class hvylya::filters::PllGenerator<float>;
