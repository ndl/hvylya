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
ValueType normOpt(const ValueType value) {
    return value * value;
}

template <typename ScalarType>
ScalarType normOpt(std::complex<ScalarType> value) {
    return std::norm(value);
}

}

template <typename ValueType>
bool LevinsonAlgorithm<ValueType>::solve(std::vector<ValueType>& x, const std::vector<ValueType>& m, const std::vector<ValueType>& y)
{
    typedef typename hvylya::core::NumericTypeMapper<ValueType>::ScalarType ScalarType;

    const std::size_t Size = m.size();
    CHECK_GT(Size, 0);
    CHECK_EQ(y.size(), Size);

    const ScalarType Epsilon = std::numeric_limits<ScalarType>::epsilon();
    const ScalarType Epsilon2 = Epsilon * Epsilon;

    if (normOpt(m[0]) < Epsilon2)
    {
        return false;
    }

    std::vector<ValueType> f(Size), b(Size), new_f(Size), new_b(Size);
    x.resize(Size, ValueType(0));

    f[0] = b[0] = ScalarType(1) / m[0];
    x[0] = y[0] / m[0];

    for (std::size_t i = 1; i < Size; ++i)
    {
        ValueType eps_f(0), eps_b(0);
        for (std::size_t j = 0; j < i; ++j)
        {
            eps_f += m[i - j] * f[j];
            eps_b += conjOpt(m[j + 1]) * b[j];
        }

        ValueType denom = ScalarType(1) - eps_f * eps_b;
        if (normOpt(denom) < Epsilon2)
        {
            return false;
        }

        ValueType coef = ScalarType(1) / denom, coef_f = eps_f / denom, coef_b = eps_b / denom;

        for (std::size_t j = 1; j < i; ++j)
        {
            new_f[j] = coef * f[j] - coef_f * b[j - 1];
            new_b[j] = coef * b[j - 1] - coef_b * f[j];
        }

        new_f[0] = coef * f[0]; new_f[i] = -coef_f * b[i - 1];
        new_b[0] = -coef_b * f[0]; new_b[i] = coef * b[i - 1];

        std::swap(f, new_f);
        std::swap(b, new_b);

        ValueType eps_x(0);
        for (std::size_t j = 0; j < i; ++j)
        {
            eps_x += m[i - j] * x[j];
        }

        for (std::size_t j = 0; j <= i; ++j)
        {
            x[j] += (y[i] - eps_x) * b[j];
        }
    }

    return true;
}

template struct hvylya::core::LevinsonAlgorithm<float>;
template struct hvylya::core::LevinsonAlgorithm<double>;
template struct hvylya::core::LevinsonAlgorithm<std::complex<float>>;
template struct hvylya::core::LevinsonAlgorithm<std::complex<double>>;
