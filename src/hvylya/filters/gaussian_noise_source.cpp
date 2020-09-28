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

#include <hvylya/filters/gaussian_noise_source.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <class T>
GaussianNoiseSource<T>::GaussianNoiseSource(T mean, T std_dev):
    dist_(mean, std_dev)
{
}

template <class T>
void GaussianNoiseSource<T>::process(const typename Base::Inputs& /* input */, typename Base::Outputs& output)
{
    auto& output_data = std::get<0>(output);
    std::size_t output_size = std::min(Base::outputState(0).suggestedSize(), output_data.size());

    for (std::size_t i = 0; i < output_size; ++i)
    {
        output_data[i] = dist_(random_generator_);
    }

    output_data.advance(output_size);
}

template class hvylya::filters::GaussianNoiseSource<float>;
