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

#include <hvylya/filters/fm/fm_stereo_demultiplexer.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename T>
FmStereoDemultiplexer<T>::FmStereoDemultiplexer(T stereo_weight)
{
    setStereoWeight(stereo_weight);
}

template <typename T>
void FmStereoDemultiplexer<T>::setStereoWeight(T stereo_weight)
{
    stereo_weight_.store(stereo_weight);
}

template <typename T>
void FmStereoDemultiplexer<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    const auto& input_left_plus_right = std::get<0>(input);
    const auto& input_left_minus_right = std::get<1>(input);

    auto& output_left = std::get<0>(output);
    auto& output_right = std::get<1>(output);

    const std::size_t data_size = std::min(
        std::min(output_left.size(), output_right.size()),
        std::min(input_left_plus_right.size(), input_left_minus_right.size())
    );

    T stereo_weight = stereo_weight_.load();
    for (std::size_t input_index = 0; input_index < data_size; ++input_index)
    {
        T left_plus_right = input_left_plus_right[input_index];
        T left_minus_right = input_left_minus_right[input_index];

        output_left[input_index] = left_plus_right + stereo_weight * left_minus_right;
        output_right[input_index] = left_plus_right - stereo_weight * left_minus_right;
    }

    input_left_plus_right.advance(data_size);
    input_left_minus_right.advance(data_size);

    output_left.advance(data_size);
    output_right.advance(data_size);
}

template class hvylya::filters::FmStereoDemultiplexer<float>;
