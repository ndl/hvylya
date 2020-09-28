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

#include <hvylya/core/levinson.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::core;

namespace {

template <typename ValueType>
ValueType conjOpt(const ValueType value) {
    return value;
}

template <typename ScalarType>
std::complex<ScalarType> conjOpt(std::complex<ScalarType> value) {
    return std::conj(value);
}

template <typename ValueType>
void checkSolution(const std::vector<ValueType>& x, const std::vector<ValueType>& m, const std::vector<ValueType>& y)
{
    const std::size_t Size = x.size();
    std::vector<ValueType> full_m(Size * Size);

    for (std::size_t row = 0; row < Size; ++row)
    {
        for (std::size_t col = 0; col < Size; ++col)
        {
            full_m[row * Size + col] = (row >= col) ? m[row - col] : conjOpt(m[col - row]);
        }
    }

    for (std::size_t row = 0; row < Size; ++row)
    {
        ValueType sum(-y[row]);
        for (std::size_t col = 0; col < Size; ++col)
        {
            sum += full_m[row * Size + col] * x[col];
        }
        LOG(ERROR) << "x[" << row << "] = " << x[row] << ", diff = " << sum;
    }
}

}

TEST(Levinson, SmallRealSystem)
{
    const std::size_t Size = 3;
    std::vector<float> m(Size), y(Size), x;

    m[0] = 3;
    m[1] = 2;
    m[2] = 1;

    y[0] = 10;
    y[1] = 14;
    y[2] = 14;

    LevinsonAlgorithm<float>::solve(x, m, y);
    checkSolution(x, m, y);
}

TEST(Levinson, SmallComplexSystem)
{
    const std::size_t Size = 3;
    std::vector<std::complex<float>> m(Size), y(Size), x;

    m[0] = std::complex<float>(3, 1);
    m[1] = std::complex<float>(2, 2);
    m[2] = std::complex<float>(1, 3);

    y[0] = std::complex<float>(10, 5);
    y[1] = std::complex<float>(14, 7);
    y[2] = std::complex<float>(14, 7);

    LevinsonAlgorithm<std::complex<float>>::solve(x, m, y);
    checkSolution(x, m, y);
}

TEST(Levinson, LargeComplexSystem)
{
    const std::size_t Size = 24;
    std::vector<std::complex<float>> m(Size), y(Size), x;

    for (std::size_t i = 0; i < Size; ++i)
    {
        m[i] = std::complex<float>(i + 1, Size - i);
        y[i] = std::complex<float>(2 * i + 1, 3 * Size * (Size - i));
    }

    LevinsonAlgorithm<std::complex<float>>::solve(x, m, y);
    checkSolution(x, m, y);
}
