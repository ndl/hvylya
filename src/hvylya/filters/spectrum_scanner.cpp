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

#include <hvylya/filters/spectrum_scanner.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename T>
SpectrumScanner<T>::SpectrumScanner(
    std::size_t fft_size,
    std::size_t averaging_window,
    T scale
):
    fft_size_(fft_size),
    fft_size2_(fft_size / 2),
    levels_size_(fft_size + 1),
    averaging_window_(averaging_window),
    max_value_(std::numeric_limits<std::size_t>::max() / averaging_window),
    current_index_(0),
    fft_transformer_(fft_size),
    buffer_(fft_size),
    result_(fft_size),
    levels_(levels_size_),
    history_(levels_size_ * averaging_window),
    scale_(scale)
{
    samples_.store(0);
    Base::inputState(0).setRequiredSize(fft_size_);
}

template <typename T>
void SpectrumScanner<T>::reset()
{
    Base::reset();
    samples_.store(0);
    std::fill(levels_.begin(), levels_.end(), T());
}

template <typename T>
bool SpectrumScanner<T>::ready() const
{
    return samples_.load() == averaging_window_;
}

template <typename T>
T SpectrumScanner<T>::level(std::size_t index) const
{
    CHECK_LT(index, levels_.size());
    return T(levels_[index]) / averaging_window_ / scale_;
}

template <typename T>
void SpectrumScanner<T>::process(const typename Base::Inputs& input, typename Base::Outputs& /* output */)
{
    const auto& input_data = std::get<0>(input);
    std::size_t input_data_size = roundDown(input_data.size(), fft_size_);

    for (std::size_t input_index = 0; input_index < input_data_size - fft_size_; input_index += fft_size_)
    {
        std::copy(&input_data[input_index], &input_data[input_index + fft_size_], buffer_.begin());
        fft_transformer_.transform(&buffer_[0], &result_[0]);

        if (samples_.load() == averaging_window_)
        {
            for (std::size_t i = 0; i < levels_size_; ++i)
            {
                levels_[i] -= history_[current_index_ * levels_size_ + i];
            }
        }
        else
        {
            ++samples_;
        }

        for (std::size_t i = 0; i < levels_size_; ++i)
        {
            // Rearrange the results in the increasing frequency order.
            T fft_abs = std::abs((i < fft_size2_) ? result_[i + fft_size2_] : result_[i - fft_size2_]);
            std::size_t val = std::min(max_value_, std::size_t(scale_ * (fft_abs / fft_size_)));
            history_[current_index_ * levels_size_ + i] = val;
            levels_[i] += val;
        }

        current_index_ = (current_index_ + 1) % averaging_window_;
    }

    input_data.advance(input_data_size);
}

template class hvylya::filters::SpectrumScanner<float>;
