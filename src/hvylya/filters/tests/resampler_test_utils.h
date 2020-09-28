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

#pragma once

#include <hvylya/filters/tests/filter_test_utils.h>
#include <hvylya/filters/fm/fm_constants.h>

namespace hvylya {
namespace filters {
namespace tests {

enum: std::size_t
{
    ResamplerRegressionTapsCount = fm::AudioResamplerTapsCount,
    ResamplerRegressionKernelTapsCount =
        (fm::AudioResamplerTapsCount + fm::AudioResamplerInterpolationRatio - 1) /
        fm::AudioResamplerInterpolationRatio,
    ResamplerRegressionBufferSize =
        fm::AudioResamplerDecimationRatio * fm::AudioResamplerInterpolationRatio * 16 +
        ResamplerRegressionKernelTapsCount - 1
};

template <typename Filter>
class ResamplerRegressionTestGeneric:
    public FilterRegressionTestGeneric<
        Filter,
        ResamplerRegressionBufferSize
    >
{
  public:
    ResamplerRegressionTestGeneric()
    {
        std::vector<double> taps(&fm::AudioResamplerTaps[0], &fm::AudioResamplerTaps[fm::AudioResamplerTapsCount]);
        FilterRegressionTestGeneric<Filter, ResamplerRegressionBufferSize>::createTaps(taps);
    }

  protected:
    virtual VerificationInfo verificationInfo() const override
    {
        VerificationInfo info;
        info.taps_count = ResamplerRegressionTapsCount;
        info.passband = fm::AudioBandwidth / fm::IntermediateAudioSamplingRate;
        info.stopband = 0.5;
        info.freq_multiplier = double(fm::AudioResamplerDecimationRatio) / double(fm::AudioResamplerInterpolationRatio);
        info.phase_shift = (ResamplerRegressionKernelTapsCount - 1) / 2.0;
        info.accuracy = 0.0025;
        return info;
    }
};

} // namespace tests
} // namespace filters
} // namespace hvylya
