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

#include <fftw3.h>

namespace hvylya {
namespace filters {
namespace tests {

class FftEnvironment: public testing::Environment
{
  public:
    virtual void TearDown() override
    {
        fftw_cleanup();
        fftwf_cleanup();
    }
};

// Intentionally define it in header and as non-static so that every including binary has a copy of it
// and linking fails if there are two inclusions.
DISABLE_WARNING_PUSH("-Wmissing-variable-declarations")

testing::Environment* fft_env = testing::AddGlobalTestEnvironment(new FftEnvironment);

DISABLE_WARNING_POP()

} // namespace tests
} // namespace filters
} // namespace hvylya
