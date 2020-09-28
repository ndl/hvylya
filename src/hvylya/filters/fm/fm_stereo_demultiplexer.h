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

#include <hvylya/filters/filter_generic.h>

namespace hvylya {
namespace filters {

using namespace hvylya::core;

template <class T>
class FmStereoDemultiplexer:
    public FilterGeneric<
        TypeList<T, T>,
        TypeList<T, T>
    >
{
  public:
    typedef typename FilterBaseType<FmStereoDemultiplexer>::Type Base;

    FmStereoDemultiplexer(T stereo_weight = 1);

    void setStereoWeight(T stereo_weight);

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override;

  private:
    std::atomic<T> stereo_weight_;
};

} // namespace filters
} // namespace hvylya
