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

#include <hvylya/core/common.h>

namespace hvylya {
namespace filters {

template <typename T>
class SdrKernelSource;

} // namespace hvylya
} // namespace filters

namespace fm_receiver {

struct Station
{
  public:
    Station(std::size_t frequency, float snr);

    std::size_t frequency() const;

    float snr() const;

  private:
    std::size_t frequency_;
    float snr_;
};

// std::size_t max value is ~1e19 and averaging window with parameters above
// is 100000, so at the scale 1e10 we can store the values on the order of 1e5,
// which is way above what we have in practice (<= 10).
void findStations(
    std::vector<Station>& stations,
    hvylya::filters::SdrKernelSource<float>& source,
    std::size_t sampling_rate = 2000000,
    std::size_t use_spectrum_percent = 90,
    std::size_t readings_per_sec = 10,
    float snr_threshold = 8.0f,
    float scale = 1e10f
);

} // namespace fm_receiver
