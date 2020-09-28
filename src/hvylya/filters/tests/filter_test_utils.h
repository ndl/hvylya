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

#include <hvylya/core/aligned_vector.h>
#include <hvylya/core/slice.h>

#include <hvylya/core/tests/common.h>

namespace hvylya {
namespace filters {
namespace tests {

template <typename SampleType>
struct FilterRegressionTraits
{
    static SampleType createSample(SampleType value)
    {
        return value;
    }

    static SampleType getAmp(SampleType value)
    {
        return std::abs(value);
    }
};

template <typename ScalarType>
struct FilterRegressionTraits<std::complex<ScalarType>>
{
    typedef std::complex<ScalarType> SampleType;

    static SampleType createSample(ScalarType value)
    {
        return std::complex<ScalarType>(value, 0.5f * value);
    }

    static ScalarType getAmp(SampleType value)
    {
        return std::abs(0.5f * value.real() + value.imag());
    }
};

struct VerificationInfo
{
    std::size_t taps_count;
    double passband, stopband, freq_multiplier, phase_shift, accuracy;
};

template <typename Filter, std::size_t BufferSize = 8192>
class FilterRegressionTestGeneric: public testing::Test
{
  private:
    typedef typename Filter::FirTypes FirTypes;
    typedef typename FirTypes::ScalarType ScalarType;
    typedef typename FirTypes::SampleType SampleType;
    typedef typename FirTypes::TapType TapType;

  public:
    template <typename... Args>
    void run(std::size_t iterations, Args... args)
    {
        core::AlignedVector<SampleType> input_vector(BufferSize + Filter::Padding), output_vector(BufferSize);
        // Fill padding.
        std::fill(input_vector.begin() + BufferSize, input_vector.end(), 0);

        for (double freq = 0.0; freq <= 0.5; freq += 0.01)
        {
            auto filter = std::make_unique<Filter>(&taps_[0], args...);

            for (std::size_t i = 0; i < BufferSize; ++i)
            {
                input_vector[i] = FilterRegressionTraits<SampleType>::createSample(ScalarType(std::cos(freq * i * 2 * M_PI)));
            }

            std::size_t advanced_size = 0;

            // We don't want the amplitude check below to significantly
            // influence timings, so perform the filtering multiple times before
            // checking the result.
            for (std::size_t i = 0; i < iterations; ++i)
            {
                core::Slice<SampleType> input(&input_vector[0], BufferSize), output(output_vector);

                auto input_tuple = std::make_tuple(std::cref(input));
                auto output_tuple = std::make_tuple(std::ref(output));

                filter->process(input_tuple, output_tuple);

                advanced_size = output.advancedSize();
            }

            VerificationInfo info = verificationInfo();

            double out_amps = 0.0, amps_diff = 0.0;
            for (std::size_t i = 0; i < advanced_size; ++i)
            {
                double out_amp = FilterRegressionTraits<SampleType>::getAmp(output_vector[i]);
                double in_amp = FilterRegressionTraits<SampleType>::getAmp(
                     FilterRegressionTraits<SampleType>::createSample(
                         ScalarType(std::cos(freq * 2 * M_PI * (info.freq_multiplier * i + info.phase_shift)))
                     )
                );

                out_amps += out_amp / advanced_size;
                amps_diff += std::abs(out_amp - in_amp) / advanced_size;
            }

            //LOG(ERROR) << "freq = " << freq << ", out_amps = " << out_amps << ", amps diff = " << amps_diff;
            if (freq <= info.passband)
            {
                EXPECT_NEAR(0.0, amps_diff, info.accuracy);
            }
            else if (freq > info.passband && freq <= info.stopband)
            {
                // Transition zone - exact check is difficult,
                // so just check the range is OK.
                EXPECT_GT(amps_diff, 0.0);
                EXPECT_LT(amps_diff, 1.0);
            }
            else
            {
                EXPECT_NEAR(0.0, out_amps, info.accuracy);
            }
        }
    }

  protected:
    void createTaps(const std::vector<double>& taps)
    {
        ASSERT_EQ(verificationInfo().taps_count, taps.size());
        taps_.resize(taps.size());
        std::copy(taps.begin(), taps.end(), taps_.begin());
    }

    virtual VerificationInfo verificationInfo() const = 0;

  private:
    std::vector<TapType> taps_;
};

} // namespace tests
} // namespace filters
} // namespace hvylya
