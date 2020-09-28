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

#include <hvylya/filters/fm/rds_bits_decoder.h>

using namespace hvylya::core;
using namespace hvylya::filters;

#ifdef RDS_DUMP_CLOCK_STATS
std::size_t g_clocks_stats[16 * 4] = { 0 };
#endif // RDS_DUMP_CLOCK_STATS

// #define USE_LOCAL_SEARCH

namespace {

const std::size_t AveragingWindowSize = 256;
const std::size_t GroupBitsCount = 4 * 26;

#ifdef USE_LOCAL_SEARCH
const std::size_t LocalSearchSize = 20;
#endif // USE_LOCAL_SEARCH

} // anonymous namespace

template <typename T>
RdsBitsDecoder<T>::RdsBitsDecoder()
{
    reset();

    for (std::size_t clock_index = 0; clock_index < FrequencyDivider; ++clock_index)
    {
        mags_sums_[clock_index].setSize(AveragingWindowSize);
    }

    // Use smaller buffering for our output.
    Base::outputState(0).setSuggestedSize(GroupBitsCount);
    // If we don't lock on RDS symbols, we won't have any output - let
    // pipeline know about it.
    Base::outputState(0).setProvidedSize(0);
}

template <typename T>
void RdsBitsDecoder<T>::reset()
{
    Base::reset();
    prev_pll_value_ = 0;
    prev_rds_value_ = 0;
    prev_sign_real_ = 0;
    prev_sign_imag_ = 0;
    prev_symbol_ = 0;
    clock_counter_ = 0;
    clocks_passed_ = FrequencyDivider;
    best_clock_ = 0;
}

template <typename T>
void RdsBitsDecoder<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    const auto& input_rds_data = std::get<0>(input);
    const auto& input_pll_data = std::get<1>(input);
    auto& output_data = std::get<0>(output);
#ifdef RDS_DUMP_BITS
    auto& output_bits_data = std::get<1>(output);
#endif

    const std::size_t input_data_size = std::min(input_rds_data.size(), input_pll_data.size());
    const std::size_t output_data_size = output_data.size();

    std::size_t input_index = 0, output_index = 0;
#ifdef RDS_DUMP_BITS
    std::size_t output_bits_data_size = output_bits_data.size(), output_bits_index = 0;
    for (; input_index < input_data_size && output_index < output_data_size && output_bits_index < output_bits_data_size; ++input_index, ++output_bits_index)
#else
    for (; input_index < input_data_size && output_index < output_data_size; ++input_index)
#endif
    {
        T rds_value = input_rds_data[input_index];
        std::complex<T> pll_value = input_pll_data[input_index];

#ifdef RDS_DUMP_BITS
        T current_debug_bit = 0;
#endif

        int sign_real = core::sign(pll_value.real());
        int sign_imag = core::sign(pll_value.imag());

        if (prev_sign_real_ + sign_real == 0 || prev_sign_imag_ + sign_imag == 0 || sign_real == 0 || sign_imag == 0)
        {
            T best_rds_value = 
                (std::min(std::abs(pll_value.real()), std::abs(pll_value.imag())) <
                 std::min(std::abs(prev_pll_value_.real()), std::abs(prev_pll_value_.imag()))) ?
                rds_value :
                prev_rds_value_;

            T best_abs_rds_value = std::abs(best_rds_value);
            mags_sums_[clock_counter_].add(best_abs_rds_value);

            if (best_clock_ == clock_counter_ &&
                clocks_passed_ > FrequencyDivider / 2
            )
            {
#ifdef USE_LOCAL_SEARCH
                for (std::size_t offset = 1; offset < LocalSearchSize; ++offset)
                {
                    if (offset < input_index &&
                        core::sign(input_rds_data[input_index - offset - 1] - input_rds_data[input_index - offset]) ==
                        core::sign(input_rds_data[input_index - offset + 1] - input_rds_data[input_index - offset]))
                    {
                        best_rds_value = input_rds_data[input_index - offset];
                        break;
                    }

                    if (input_index + offset + 1 < input_data_size &&
                        core::sign(input_rds_data[input_index + offset - 1] - input_rds_data[input_index + offset]) ==
                        core::sign(input_rds_data[input_index + offset + 1] - input_rds_data[input_index + offset]))
                    {
                        best_rds_value = input_rds_data[input_index + offset];
                        break;
                    }
                }
#endif // USE_LOCAL_SEARCH

                int current_symbol = best_rds_value > 0 ? 1 : 0;
                int output_symbol = prev_symbol_ ^ current_symbol;
                prev_symbol_ = current_symbol;
                output_data[output_index++] = std::int8_t(output_symbol);
#ifdef RDS_DUMP_CLOCK_STATS
                ++g_clocks_stats[best_clock_];
#endif // RDS_DUMP_CLOCK_STATS
                clocks_passed_ = 0;
#ifdef RDS_DUMP_BITS
                current_debug_bit = current_symbol ? best_abs_rds_value : -best_abs_rds_value;
#endif
            }

            ++clock_counter_;
            ++clocks_passed_;

            if (clock_counter_ == FrequencyDivider)
            {
                clock_counter_ = 0;
                best_clock_ = selectBestClock();
            }
        }

        prev_rds_value_ = rds_value;
        prev_pll_value_ = pll_value;
        prev_sign_real_ = sign_real;
        prev_sign_imag_ = sign_imag;
#ifdef RDS_DUMP_BITS
        output_bits_data[output_bits_index] = current_debug_bit;
#endif
    }

    input_rds_data.advance(input_index);
    input_pll_data.advance(input_index);
    output_data.advance(output_index);
#ifdef RDS_DUMP_BITS
    output_bits_data.advance(output_bits_index);
#endif
}

template <typename T>
std::size_t RdsBitsDecoder<T>::selectBestClock() const
{
    T best_mag = T();
    std::size_t best_clock = 0;

    for (std::size_t clock_index = 0; clock_index < FrequencyDivider; ++clock_index)
    {
        if (!mags_sums_[clock_index].empty())
        {
            T avg = mags_sums_[clock_index].avg();

            if (avg > best_mag)
            {
                best_mag = avg;
                best_clock = clock_index;
            }
        }
    }

    return best_clock;
}

template class hvylya::filters::RdsBitsDecoder<float>;
