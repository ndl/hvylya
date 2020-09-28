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

#include <hvylya/filters/sampler.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename T>
Sampler<T>::Sampler(std::size_t rate, std::size_t window_size):
    rate_(rate),
    window_size_(window_size),
    remaining_skip_(0),
    remaining_output_(window_size)
{
    if (window_size_ == 1)
    {
        Base::inputState(0).setRequiredSize(rate_);
    }
    else
    {
        // We cannot guarantee any output in the windowed mode,
        // as to do it we'd need to require window_size_ * rate_
        // samples at the input, which might be too large.
        Base::outputState(0).setProvidedSize(0);
    }
}

template <typename T>
void Sampler<T>::reset()
{
    Base::reset();
    remaining_skip_ = 0;
    remaining_output_ = window_size_;
}

template <typename T>
void Sampler<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    const auto& input_data = std::get<0>(input);
    auto& output_data = std::get<0>(output);

    if (window_size_ == 1)
    {
        const std::size_t input_data_size =
            roundDown(
                std::min(input_data.size(), output_data.size() * rate_),
                rate_
            );
        const std::size_t output_data_size = input_data_size / rate_;

        for (std::size_t input_index = 0, output_index = 0; input_index < input_data_size; input_index += rate_, ++output_index)
        {
            output_data[output_index] = input_data[input_index];
        }

        input_data.advance(input_data_size);
        output_data.advance(output_data_size);
    }
    else
    {
        std::size_t input_index = 0, output_index = 0;

        while (input_index < input_data.size() && output_index < output_data.size())
        {
            CHECK_NE(remaining_skip_ == 0, remaining_output_ == 0);

            if (remaining_skip_ > 0)
            {
                std::size_t offset = std::min(remaining_skip_, input_data.size() - input_index);
                remaining_skip_ -= offset;
                input_index += offset;

                if (!remaining_skip_)
                {
                    remaining_output_ = window_size_;
                }
            }
            else if (remaining_output_ > 0)
            {
                std::size_t offset = std::min(
                    std::min(remaining_output_, input_data.size() - input_index),
                    output_data.size() - output_index
                );

                for (
                    std::size_t input_index_end = input_index + offset;
                    input_index < input_index_end;
                    ++input_index, ++output_index
                )
                {
                    output_data[output_index] = input_data[input_index];
                }

                remaining_output_ -= offset;

                if (!remaining_output_)
                {
                    remaining_skip_ = window_size_ * (rate_ - 1);
                }
            }
        }

        input_data.advance(input_index);
        output_data.advance(output_index);
    }
}

template class hvylya::filters::Sampler<float>;
template class hvylya::filters::Sampler<std::complex<float>>;
