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

#include <fm-receiver/stations.h>

#include <hvylya/filters/connect.h>
#include <hvylya/filters/sdr_kernel_source.h>
#include <hvylya/filters/spectrum_scanner.h>

#include <hvylya/pipelines/async/pipeline.h>

using namespace hvylya::filters;
using namespace hvylya::pipelines::async;

using namespace fm_receiver;

Station::Station(std::size_t frequency, float snr):
    frequency_(frequency),
    snr_(snr)
{
}

std::size_t Station::frequency() const
{
    return frequency_;
}

float Station::snr() const
{
    return snr_;
}

void fm_receiver::findStations(
    std::vector<Station>& stations,
    SdrKernelSource<float>& source,
    std::size_t sampling_rate,
    std::size_t use_spectrum_percent,
    std::size_t readings_per_sec,
    float snr_threshold,
    float scale
)
{
    const std::size_t MinFmFreq = 87500000;
    const std::size_t MaxFmFreq = 108000000;
    const std::size_t FmFreqInterval = 100000;

    const std::size_t SamplingRate2 = sampling_rate / 2;
    const std::size_t FftSize = sampling_rate / FmFreqInterval;
    const std::size_t AveragingWindow = sampling_rate / FftSize / readings_per_sec;

    useconds_t SleepInterval = 1000000 / readings_per_sec / 2;

    const std::size_t UnusedSpectrumPercent = 100 - use_spectrum_percent;

    std::unordered_map<std::size_t, float> candidates;

    auto cmp =
        [&](const auto& station0, const auto& station1)
        {
            return station0.snr() < station1.snr();
        };
    std::priority_queue<Station, std::vector<Station>, decltype(cmp)> candidates_queue(cmp);

    source.setSamplingRate(sampling_rate);
    SpectrumScanner<float> scanner(FftSize, AveragingWindow, scale);
    connect(source, scanner);
    Pipeline pipeline;
    pipeline.add(source);

    std::size_t freq_step = use_spectrum_percent * sampling_rate / 100;
    std::size_t freq_step2 = freq_step / 2;
    std::size_t offset = UnusedSpectrumPercent * FftSize / 100 / 2;

    for (std::size_t freq = MinFmFreq + freq_step2; freq - freq_step2 < MaxFmFreq; freq += freq_step)
    {
        source.setCenterFrequency(freq);

        scanner.reset();
        source.reset();
        pipeline.resume();

        while (!scanner.ready())
        {
            usleep(SleepInterval);
        }

        pipeline.pause();

        float min_level = std::numeric_limits<float>::max();
        for (std::size_t i = offset; i < FftSize - offset; ++i)
        {
            min_level = std::min(min_level, scanner.level(i));
        }

        for (std::size_t i = offset; i < FftSize - offset; ++i)
        {
            float snr = 20 * std::log10(scanner.level(i) / min_level);

            if (snr > snr_threshold)
            {
                std::size_t station_freq = freq - SamplingRate2 + i * sampling_rate / FftSize;
                candidates[station_freq] = snr;
                candidates_queue.emplace(station_freq, snr);
            }
        }
    }

    pipeline.stop();

    while (!candidates_queue.empty())
    {
        Station candidate = candidates_queue.top();
        candidates_queue.pop();

        if (candidates.count(candidate.frequency()))
        {
            std::size_t left_freq = candidate.frequency() - FmFreqInterval;
            std::size_t right_freq = candidate.frequency() + FmFreqInterval;

            if ((!candidates.count(left_freq) || candidates[left_freq] < candidate.snr()) &&
                (!candidates.count(right_freq) || candidates[right_freq] < candidate.snr()))
            {
                stations.push_back(candidate);
                candidates.erase(left_freq);
                candidates.erase(right_freq);
            }

            candidates.erase(candidate.frequency());
        }
    }

    std::sort(
        stations.begin(),
        stations.end(),
        [](const auto& station0, const auto& station1)
        {
            return station0.frequency() < station1.frequency();
        }
    );
}
