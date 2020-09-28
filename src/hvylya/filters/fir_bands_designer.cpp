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

#include <hvylya/filters/fir_bands_designer.h>

using namespace hvylya::filters;

template <typename T>
void FirBandsDesigner<T>::createLowpassBands(
    std::vector<ConstFirBand<T>>& bands,
    std::vector<const IFirBand<T>*>& bands_ptrs,
    T passband,
    T stopband,
    T gain
)
{
    CHECK(passband > 0);
    CHECK(stopband > passband);
    CHECK(stopband <= 0.5);

    bands.resize(2);
    bands_ptrs.resize(2);

    bands[0] = ConstFirBand<T>(0.0, 2 * M_PI * passband, gain, 1);
    bands[1] = ConstFirBand<T>(2 * M_PI * stopband, M_PI, 0, 1);

    bands_ptrs[0] = &bands[0];
    bands_ptrs[1] = &bands[1];
}

template <typename T>
void FirBandsDesigner<T>::createHighpassBands(
    std::vector<ConstFirBand<T>>& bands,
    std::vector<const IFirBand<T>*>& bands_ptrs,
    T stopband,
    T passband,
    T gain
)
{
    CHECK(stopband >= 0);
    CHECK(stopband < passband);
    CHECK(passband < 0.5);

    bands.resize(2);
    bands_ptrs.resize(2);

    bands[0] = ConstFirBand<T>(0.0, 2 * M_PI * stopband, 0, 1);
    bands[1] = ConstFirBand<T>(2 * M_PI * passband, M_PI, gain, 1);

    bands_ptrs[0] = &bands[0];
    bands_ptrs[1] = &bands[1];
}

template <typename T>
void FirBandsDesigner<T>::createBandpassBands(
    std::vector<ConstFirBand<T>>& bands,
    std::vector<const IFirBand<T>*>& bands_ptrs,
    T left_stopband,
    T left_passband,
    T right_passband,
    T right_stopband,
    T gain
)
{
    CHECK(left_stopband > 0);
    CHECK(left_passband > left_stopband);
    CHECK(right_passband > left_passband);
    CHECK(right_stopband > right_passband);
    CHECK(right_stopband < 0.5);

    bands.resize(3);
    bands_ptrs.resize(3);

    bands[0] = ConstFirBand<T>(0.0, 2 * M_PI * left_stopband, 0, 1);
    bands[1] = ConstFirBand<T>(2 * M_PI * left_passband, 2 * M_PI * right_passband, gain, 1);
    bands[2] = ConstFirBand<T>(2 * M_PI * right_stopband, M_PI, 0, 1);

    bands_ptrs[0] = &bands[0];
    bands_ptrs[1] = &bands[1];
    bands_ptrs[2] = &bands[2];
}

template <typename T>
void FirBandsDesigner<T>::createHilbertBandpassBands(
    std::vector<ConstFirBand<T>>& bands,
    std::vector<const IFirBand<T>*>& bands_ptrs,
    T left_passband,
    T right_passband,
    T gain
)
{
    CHECK(left_passband > 0);
    CHECK(right_passband > left_passband);
    CHECK(right_passband < 0.5);

    bands.resize(1);
    bands_ptrs.resize(1);

    bands[0] = ConstFirBand<T>(2 * M_PI * left_passband, 2 * M_PI * right_passband, gain, 1);

    bands_ptrs[0] = &bands[0];
}

template <typename T>
void FirBandsDesigner<T>::createHilbertHighpassBands(
    std::vector<ConstFirBand<T>>& bands,
    std::vector<const IFirBand<T>*>& bands_ptrs,
    T passband,
    T gain
)
{
    CHECK(passband > 0);
    CHECK(passband < 0.5);

    bands.resize(1);
    bands_ptrs.resize(1);

    bands[0] = ConstFirBand<T>(2 * M_PI * passband, M_PI, gain, 1);

    bands_ptrs[0] = &bands[0];
}

template class hvylya::filters::FirBandsDesigner<double>;
template class hvylya::filters::FirBandsDesigner<long double>;
