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
namespace core {

template <typename T>
class LagrangeInterpolator
{
  public:
    LagrangeInterpolator(T scale): scale_(scale) { }

    LagrangeInterpolator(const std::vector<T>& args, const std::vector<T>& values, T scale):
        scale_(scale)
    {
        CHECK_EQ(args.size(), values.size());

        updateArguments(args);
        updateValues(values);
    }

    void updateArguments(const std::vector<T>& args)
    {
        CHECK(!args.empty());

        nodes_.resize(args.size());
        weights_.resize(args.size());

        for (std::size_t i = 0; i < args.size(); ++i)
        {
            weights_[i] = 1;
            for (std::size_t j = 0; j < args.size(); ++j)
            {
                if (i != j)
                {
                    weights_[i] *= scale_ * (args[i] - args[j]);
                }
            }
            weights_[i] = 1 / weights_[i];
            nodes_[i].first = args[i];
        }
    }

    void updateValue(std::size_t index, T value)
    {
        CHECK(index < nodes_.size());
        nodes_[index].second = value;
    }

    void updateValues(const std::vector<T>& values)
    {
        CHECK_EQ(nodes_.size(), values.size());

        for (std::size_t i = 0; i < nodes_.size(); ++i)
        {
            nodes_[i].second = values[i];
        }
    }

    const std::vector<T>& weights() const
    {
        return weights_;
    }

    T scale() const
    {
        return scale_;
    }

    T evaluate(T arg) const
    {
        std::size_t skip_index = closestNodeIndex(arg);
        T w = weights_[skip_index];
        T f = nodes_[skip_index].second;
        T t = arg - nodes_[skip_index].first;
        T n = numerator(arg, skip_index);
        T d = denominator(arg, skip_index);
        T denom = t * d + w;
        return (t * n + f * w) / denom;
    }

    T evaluateDerivative(T arg) const
    {
        std::size_t skip_index = closestNodeIndex(arg);
        T w = weights_[skip_index];
        T f = nodes_[skip_index].second;
        T t = arg - nodes_[skip_index].first;
        T n = numerator(arg, skip_index);
        T d = denominator(arg, skip_index);
        T n1 = numeratorDerivative(arg, skip_index);
        T d1 = denominatorDerivative(arg, skip_index);
        T denom = t * d + w;
        return
            (t * t * (d * n1 - d1 * n) +
             w * (t * (n1 - f * d1) +
                  (n - f * d))) /
            (denom * denom);
    }

    T evaluateSecondDerivative(T arg) const
    {
        std::size_t skip_index = closestNodeIndex(arg);
        T w = weights_[skip_index];
        T f = nodes_[skip_index].second;
        T t = arg - nodes_[skip_index].first;
        T n = numerator(arg, skip_index);
        T d = denominator(arg, skip_index);
        T n1 = numeratorDerivative(arg, skip_index);
        T d1 = denominatorDerivative(arg, skip_index);
        T n2 = numeratorSecondDerivative(arg, skip_index);
        T d2 = denominatorSecondDerivative(arg, skip_index);
        T denom = t * d + w, tt = t * t, d1d1 = d1 * d1, d1n1 = d1 * n1, dn2 = d * n2;
        return
            (tt * t * (d * (dn2 - 2 * d1n1 - d2 * n) + 2 * d1d1 * n) +
             w * (tt * (2 * (dn2 - d1n1 + f * d1d1) - d2 * (n + f * d)) +
                  t * (w * n2 + 2 * d * n1 - 4 * d1 * n + f * (2 * d * d1 - w * d2)) +
                  2 * (w * n1 - d * n + f * (d * d - w * d1)))) /
            (denom * denom * denom);
    }

    T evaluateValueToDerivativeRatio(T arg) const
    {
        std::size_t skip_index = closestNodeIndex(arg);
        T w = weights_[skip_index];
        T f = nodes_[skip_index].second;
        T t = arg - nodes_[skip_index].first;
        T n = numerator(arg, skip_index);
        T d = denominator(arg, skip_index);
        T n1 = numeratorDerivative(arg, skip_index);
        T d1 = denominatorDerivative(arg, skip_index);
        T denom = t * d + w;
        return
            denom * (t * n + f * w) /
            (t * t * (d * n1 - d1 * n) +
             w * (t * (n1 - f * d1) +
                  (n - f * d)));
    }

    T evaluateDerivativeToSecondDerivativeRatio(T arg) const
    {
        std::size_t skip_index = closestNodeIndex(arg);
        T w = weights_[skip_index];
        T f = nodes_[skip_index].second;
        T t = arg - nodes_[skip_index].first;
        T n = numerator(arg, skip_index);
        T d = denominator(arg, skip_index);
        T n1 = numeratorDerivative(arg, skip_index);
        T d1 = denominatorDerivative(arg, skip_index);
        T n2 = numeratorSecondDerivative(arg, skip_index);
        T d2 = denominatorSecondDerivative(arg, skip_index);
        T denom = t * d + w, tt = t * t, d1d1 = d1 * d1, d1n1 = d1 * n1, dn2 = d * n2;
        return
            denom * (t * t * (d * n1 - d1 * n) +
                     w * (t * (n1 - f * d1) +
                          (n - f * d))) /
            (tt * t * (d * (dn2 - 2 * d1n1 - d2 * n) + 2 * d1d1 * n) +
             w * (tt * (2 * (dn2 - d1n1 + f * d1d1) - d2 * (n + f * d)) +
                  t * (w * n2 + 2 * d * n1 - 4 * d1 * n + f * (2 * d * d1 - w * d2)) +
                  2 * (w * n1 - d * n + f * (d * d - w * d1))));
    }

  private:
    std::vector<T> weights_;
    std::vector<std::pair<T, T>> nodes_;
    T scale_;

    std::size_t closestNodeIndex(T arg) const
    {
        std::size_t closest_node_index = 0;
        T closest_node_dist = std::abs(arg - nodes_[closest_node_index].first);

        for (std::size_t i = 1; i < nodes_.size(); ++i)
        {
            T node_dist = std::abs(arg - nodes_[i].first);
            if (node_dist < closest_node_dist)
            {
                closest_node_index = i;
                closest_node_dist = node_dist;
            }
        }

        return closest_node_index;
    }

    T numerator(T arg, std::size_t skip_index) const
    {
        T num = 0;
        for (std::size_t i = 0; i < nodes_.size(); ++i)
        {
            if (i != skip_index)
            {
                num += weights_[i] * nodes_[i].second / (arg - nodes_[i].first);
            }
        }

        return num;
    }

    T denominator(T arg, std::size_t skip_index) const
    {
        T denom = 0;
        for (std::size_t i = 0; i < nodes_.size(); ++i)
        {
            if (i != skip_index)
            {
                denom += weights_[i] / (arg - nodes_[i].first);
            }
        }
        return denom;
    }

    T numeratorDerivative(T arg, std::size_t skip_index) const
    {
        T num = 0;
        for (std::size_t i = 0; i < nodes_.size(); ++i)
        {
            if (i != skip_index)
            {
                T d = arg - nodes_[i].first;
                num -= weights_[i] * nodes_[i].second / (d * d);
            }
        }

        return num;
    }

    T denominatorDerivative(T arg, std::size_t skip_index) const
    {
        T denom = 0;
        for (std::size_t i = 0; i < nodes_.size(); ++i)
        {
            if (i != skip_index)
            {
                T d = arg - nodes_[i].first;
                denom -= weights_[i] / (d * d);
            }
        }
        return denom;
    }

    T numeratorSecondDerivative(T arg, std::size_t skip_index) const
    {
        T num = 0;
        for (std::size_t i = 0; i < nodes_.size(); ++i)
        {
            if (i != skip_index)
            {
                T d = arg - nodes_[i].first;
                num += 2 * weights_[i] * nodes_[i].second / (d * d * d);
            }
        }

        return num;
    }

    T denominatorSecondDerivative(T arg, std::size_t skip_index) const
    {
        T denom = 0;
        for (std::size_t i = 0; i < nodes_.size(); ++i)
        {
            if (i != skip_index)
            {
                T d = arg - nodes_[i].first;
                denom += 2 * weights_[i] / (d * d * d);
            }
        }
        return denom;
    }
};

} // namespace core
} // namespace hvylya
