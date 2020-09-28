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

#include <hvylya/filters/fm/fm_decoder.h>

#include <hvylya/core/approx_trigonometry.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename T>
FmDecoder<T>::FmDecoder(uint32_t sample_rate):
    sample_rate_(sample_rate),
    //dc_(1000000),
    //total_sum_(0),
    min_value_(std::numeric_limits<T>::max()),
    max_value_(std::numeric_limits<T>::min())
{
    CHECK_NE(sample_rate, 0) << "sample_rate cannot be zero";

    Base::inputState(0).setHistorySize(1);
    Base::inputState(0).setRequiredSize(2 * ComplexVector::Elements);
    Base::outputState(0).setRequiredSize(ScalarVector::Elements);
}

template <typename T>
void FmDecoder<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    typedef ApproxTrigonometrySimdFromVector<ComplexVector> ApproxTrig;

    const auto& input_data = std::get<0>(input);
    auto& output_data = std::get<0>(output);

    const std::size_t output_data_size = std::min(
        roundDown(output_data.size(), ScalarVector::Elements),
        roundDown(input_data.size() - 1, 2 * ComplexVector::Elements)
    );
    // Take into account one sample from history.
    const std::size_t input_data_size = output_data_size + 1;

    const T fm_bandwidth = 75000;
    // Because our atan2 implementation is normalized in [-2, 2] range,
    // we replace 2 * M_PI by 4 in gain calculation.
    T fm_gain = sample_rate_ / (4 * fm_bandwidth);

    for (std::size_t i = 1; i < input_data_size; i += 2 * ComplexVector::Elements)
    {
        ComplexVector result0 = multiplyConjugated(
            *reinterpret_cast<const ComplexVector*>(&input_data[i]),
            *reinterpret_cast<const ComplexVector*>(&input_data[i - 1])
        );
        ComplexVector result1 = multiplyConjugated(
            *reinterpret_cast<const ComplexVector*>(&input_data[i + ComplexVector::Elements]),
            *reinterpret_cast<const ComplexVector*>(&input_data[i + ComplexVector::Elements - 1])
        );
        //ScalarVector result = ApproxTrig::normalized_atan2(result0, result1);
        *reinterpret_cast<ScalarVector*>(&output_data[i - 1]) = fm_gain * ApproxTrig::normalized_atan2(result0, result1);

        //for (std::size_t j = 0; j < ScalarVector::Elements; ++j)
        //{
        //    std::complex<T> diff(input_data[i + j] * std::conj(input_data[i + j - 1]));
        //    total_sum_ += std::atan2(diff.imag(), diff.real());
        //    output_data[i + j - 1] = T(total_sum_);
        //    total_sum_ *= 0.9999;
        //    dc_.add(output_data[i + j - 1]);
        //    min_value_ = std::min(min_value_, output_data[i + j - 1]);
        //    max_value_ = std::max(max_value_, output_data[i + j - 1]);
        //}

        //static std::size_t counter = 0;
        //if (!(counter % 1000000))
        //{
        //    LOG(ERROR) << "Total sum = " << total_sum_;
        //    LOG(ERROR) << "Current DC = " << dc_.avg();
        //    LOG(ERROR) << "Min = " << min_value_;
        //    LOG(ERROR) << "Max = " << max_value_;
        //    min_value_ = std::numeric_limits<T>::max();
        //    max_value_ = std::numeric_limits<T>::min();
        //}
        //++counter;
    }

    input_data.advance(input_data_size - 1);
    output_data.advance(output_data_size);
}

template class hvylya::filters::FmDecoder<float>;
