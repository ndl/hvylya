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

#include <hvylya/filters/fft_translating_filter.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename SampleType, typename TapType>
FftTranslatingFilter<SampleType, TapType>::FftTranslatingFilter(
    const TapType taps[],
    const std::size_t taps_count,
    ScalarType center_frequency,
    bool compensate_delay,
    std::size_t decimation_rate
):
    Base(taps_count, compensate_delay, decimation_rate),
    rotator_(center_frequency, decimation_rate)
{
    // Alignment is not so much a problem here, but we need to pad taps with zeros anyway,
    // plus conversion to ResultType is required.
    AlignedVector<std::complex<ScalarType>> new_taps(Base::block_size_);
    rotator_.createTaps(&new_taps[0], taps, taps_count);

    Base::fft_algorithm_ =
        std::make_unique<FftwConvolver<SampleType, std::complex<ScalarType>>>(
            &new_taps[0],
            Base::block_size_,
            &Base::history_[0],
            &Base::transformed_samples_[0]
        );
}

template <typename SampleType, typename TapType>
void FftTranslatingFilter<SampleType, TapType>::reset()
{
    Base::reset();
    rotator_.reset();
}

template <typename SampleType, typename TapType>
void FftTranslatingFilter<SampleType, TapType>::postProcess(ResultType* output, std::size_t output_size)
{
    rotator_.rotate(output, output_size);
}

template class hvylya::filters::FftTranslatingFilter<float, float>;
//template class hvylya::filters::FftTranslatingFilter<float, std::complex<float>>;
template class hvylya::filters::FftTranslatingFilter<std::complex<float>, float>;
//template class hvylya::filters::FftTranslatingFilter<std::complex<float>, std::complex<float>>;
