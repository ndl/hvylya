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

#include <hvylya/filters/fm/fm_stereo_extractor.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename T>
FmStereoExtractor<T>::FmStereoExtractor()
{
}

template <typename T>
void FmStereoExtractor<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    const auto& input_pll_data = std::get<0>(input);
    const auto& input_stereo_data = std::get<1>(input);
    auto& output_data = std::get<0>(output);

    const std::size_t data_size = std::min(
        output_data.size(),
        std::min(input_pll_data.size(), input_stereo_data.size())
    );

    for (std::size_t input_index = 0; input_index < data_size; ++input_index)
    {
        std::complex<T> pll_value = input_pll_data[input_index];
        output_data[input_index] = (pll_value * pll_value).imag() * input_stereo_data[input_index];
    }

    input_pll_data.advance(data_size);
    input_stereo_data.advance(data_size);
    output_data.advance(data_size);
}

template class hvylya::filters::FmStereoExtractor<float>;
