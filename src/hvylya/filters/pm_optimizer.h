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

#include <hvylya/core/lagrange_interpolator.h>
#include <hvylya/filters/ifir_band.h>

namespace hvylya {
namespace filters {

enum class PmStatus
{
    Succeeded,
    IterationsLimitExceeded
};

template <typename T>
struct PmResult
{
    PmStatus status;
    std::size_t iterations;
    T delta, max_error;
};

template <typename T>
class PmOptimizer: core::NonCopyable
{
  public:
    PmOptimizer(std::size_t max_iterations = 100, T accuracy = 1e-8, std::size_t grid_density = 32);

    ~PmOptimizer();

    PmResult<T> findTaps(std::vector<T>& taps, std::size_t taps_count, const std::vector<const IFirBand<T>*>& bands, bool anti_symmetric = false);

  private:
    struct PmOptimizerImpl;
    std::unique_ptr<PmOptimizerImpl> pimpl_;
};

} // namespace hvylya
} // namespace filters
