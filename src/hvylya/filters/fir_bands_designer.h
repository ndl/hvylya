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

#include <hvylya/filters/fir_bands.h>

namespace hvylya {
namespace filters {

template <typename T>
class FirBandsDesigner: core::NonCopyable
{
  public:
    static void createLowpassBands(
        std::vector<ConstFirBand<T>>& bands,
        std::vector<const IFirBand<T>*>& bands_ptrs,
        T passband,
        T stopband,
        T gain = 1
    );

    static void createHighpassBands(
        std::vector<ConstFirBand<T>>& bands,
        std::vector<const IFirBand<T>*>& bands_ptrs,
        T stopband,
        T passband,
        T gain = 1
    );

    static void createBandpassBands(
        std::vector<ConstFirBand<T>>& bands,
        std::vector<const IFirBand<T>*>& bands_ptrs,
        T left_stopband,
        T left_passband,
        T right_passband,
        T right_stopband,
        T gain = 1
    );

    static void createHilbertBandpassBands(
        std::vector<ConstFirBand<T>>& bands,
        std::vector<const IFirBand<T>*>& bands_ptrs,
        T left_passband,
        T right_passband,
        T gain = 1
    );

    static void createHilbertHighpassBands(
        std::vector<ConstFirBand<T>>& bands,
        std::vector<const IFirBand<T>*>& bands_ptrs,
        T passband,
        T gain = 1
    );
};

} // namespace hvylya
} // namespace filters
