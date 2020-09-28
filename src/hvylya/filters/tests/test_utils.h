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
#include <hvylya/core/slice.h>

#include <hvylya/core/tests/common.h>

namespace hvylya {
namespace filters {
namespace tests {

extern const double TapsAccuracy;

template <class T>
testing::AssertionResult IsOutputExpected(
    const std::vector<T>& expected,
    const core::Slice<T>& actual,
    float tolerance
)
{
    testing::Message msg;
    std::size_t errors_found = 0;
    const char* separator = " ";

    if (expected.size() != actual.size())
    {
        msg << "Differences found: size ";
        msg << expected.size() << " != size " << actual.size();
        ++errors_found;
    }

    for (std::size_t index = 0; index < std::min(expected.size(), actual.size()); ++index)
    {
        if (fabs(expected[index] - actual[index]) > tolerance)
        {
            if (errors_found == 0)
            {
                msg << "Differences found: ";
            }

            if (errors_found < 10)
            {
                msg << separator << expected[index] << " != " << actual[index] << " @ " << index;
                separator = ", ";
            }
            ++errors_found;
        }
    }

    if (errors_found > 0)
    {
        msg << separator << errors_found << " differences in total";
        return testing::AssertionFailure(msg);
    }

    return testing::AssertionSuccess();
}

template <typename T>
void dumpTaps(const std::vector<T>& taps)
{
    for (std::size_t i = 0; i < taps.size(); ++i)
    {
        std::cout << std::setprecision(16) << taps[i] << ", ";
    }
}

template <typename T>
void checkTaps(std::initializer_list<T> expected_taps, const std::vector<T> actual_taps)
{
    EXPECT_EQ(expected_taps.size(), actual_taps.size());

    for (std::size_t i = 0; i < std::min(expected_taps.size(), actual_taps.size()); ++i)
    {
        EXPECT_NEAR(*(expected_taps.begin() + i), actual_taps[i], TapsAccuracy);
    }
}

} // namespace tests
} // namespace filters
} // namespace hvylya
