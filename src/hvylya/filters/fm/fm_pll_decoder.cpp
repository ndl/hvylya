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

#include <hvylya/filters/fm/fm_pll_decoder.h>

#include <hvylya/core/approx_trigonometry.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename T>
FmPllDecoder<T>::FmPllDecoder(std::size_t sample_rate):
    phase_(T(0)),
    phase_gain_(T(1.0)),
    fm_gain_(sample_rate / T(2 * M_PI * 75000.0))
{
    Base::inputState(0).setRequiredSize(2 * ComplexVector::Elements);
    Base::outputState(0).setRequiredSize(2 * ComplexVector::Elements);
}

template <typename T>
void FmPllDecoder<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
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

        for (std::size_t i = 0; i < ScalarVector::Elements; ++i)
        {
            T phase_error = getClampedByPi(sample_phase[i] - phase_);
            output_data[input_index + i] = fm_gain_ * phase_error;
            phase_ += phase_gain_ * phase_error;
            phase_ = getClampedByPi(phase_);
        }
    }

    input_data.advance(data_size);
    output_data.advance(data_size);
}

template <typename T>
T FmPllDecoder<T>::getClampedByPi(T value)
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

template class hvylya::filters::FmPllDecoder<float>;
