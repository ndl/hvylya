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

#include <hvylya/filters/iir_filters_designer.h>

using namespace hvylya::core;
using namespace hvylya::filters;

template <typename SampleType>
typename IirFiltersDesigner<SampleType>::IirBiquadFilter
IirFiltersDesigner<SampleType>::createLowpassBiquadFilter(ScalarType corner_freq, ScalarType bandwidth)
{
    // For Q calculation, see http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
    double q = 1.0 / (2.0 * std::sinh(log(2.0) / 2.0 * double(bandwidth) * double(corner_freq) / std::sin(double(corner_freq))));
    // For taps calculation, see http://www.st.com/st-web-ui/static/active/en/resource/technical/document/application_note/DM00038862.pdf
    // Results are idential to the one given in the 'cookbook' link above, but the latter is more complete and has
    // results for 1st order as well in the same terminology, hence using these for consistency.
    double k = std::tan(double(corner_freq) / 2), w = k * k, de = 1.0 + k / q + w;
    TapType b[3], a[3];
    b[0] = b[2] = TapType(w / de);
    b[1] = TapType(2.0 * w / de);
    a[0] = TapType(1.0);
    a[1] = TapType(2.0 * (w - 1.0) / de);
    a[2] = TapType((1.0 - k / q + w) / de);
    return IirBiquadFilter(b, a);
}

template <typename SampleType>
typename IirFiltersDesigner<SampleType>::IirFirstOrderFilter
IirFiltersDesigner<SampleType>::createLowpassFirstOrderFilter(ScalarType corner_freq)
{
    double k = std::tan(double(corner_freq) / 2), alpha = 1.0 + k;
    TapType b[2], a[2];
    b[0] = b[1] = TapType(k / alpha);
    a[0] = TapType(1.0);
    a[1] = TapType((k - 1.0) / alpha);
    return IirFirstOrderFilter(b, a);
}

template class hvylya::filters::IirFiltersDesigner<float>;
template class hvylya::filters::IirFiltersDesigner<std::complex<float>>;
