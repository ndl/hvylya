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

#include <hvylya/filters/saw_tooth_source.h>

using namespace hvylya::core;
using namespace hvylya::filters;

namespace {

template <typename T>
T createSample(T value)
{
    return value;
}

template <typename T>
std::complex<T> createSample(std::complex<T> value)
{
    return std::complex<T>(value.real(), value.real());
}

} // anonymous namespace

template <typename T>
SawToothSource<T>::SawToothSource(std::size_t samples, std::size_t saw_tooth_size, bool loop):
    samples_(samples),
    saw_tooth_size_(saw_tooth_size),
    current_sample_(0),
    output_samples_(0),
    loop_(loop)
{
    CHECK_NE(samples, 0) << "samples number must be > 0";

    for (std::size_t index = 0; index < saw_tooth_size_; ++index)
    {
        saw_tooth_samples_[index] = createSample(T(index));
    }
}

template <typename T>
void SawToothSource<T>::process(const typename Base::Inputs& /* input */, typename Base::Outputs& output)
{
    auto& output_data = std::get<0>(output);
    std::size_t output_size = std::min(Base::outputState(0).suggestedSize(), output_data.size());

    for (std::size_t index = 0; index < output_size; index += saw_tooth_size_)
    {
        std::copy(&saw_tooth_samples_[0], &saw_tooth_samples_[std::min(saw_tooth_size_, output_size - index)], &output_data[index]);
    }

    output_samples_ += output_size;

    if (output_samples_ > samples_)
    {
        if (loop_)
        {
            output_samples_ = 0;
            current_sample_ = 0;
        }
        else
        {
            Base::outputState(0).setEof(true);
        }
    }

    output_data.advance(output_size);
}

template class hvylya::filters::SawToothSource<float>;
template class hvylya::filters::SawToothSource<std::complex<float>>;
