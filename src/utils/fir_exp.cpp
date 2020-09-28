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

#include <hvylya/filters/pm_optimizer.h>
#include <hvylya/filters/fir_bands.h>

#include <iostream>

using namespace hvylya::core;
using namespace hvylya::filters;

namespace {

const std::size_t TapsCount = 17;

class DecayedFirBand: public IFirBand<double>
{
  public:
    DecayedFirBand(double min_freq, double max_freq, double base_gain):
        min_freq_(min_freq),
        max_freq_(max_freq),
        base_gain_(base_gain)
    {
    }

    virtual std::pair<double, double> frequencies() const override
    {
        return std::make_pair(min_freq_, max_freq_);
    }

    virtual double targetGain(double arg) const override
    {
        double freq = (1.0 - arg) * min_freq_ + arg * max_freq_;
        double center = (min_freq_ + max_freq_) / 2.0;
        double diff = (center - freq);
        if (diff > 0.0)
        {
            double n = hvylya::core::sqr((center - diff) / (center + diff));
            return base_gain_ * (1.0 - n / (n + 1.0));
        }
        else if (diff < 0.0)
        {
            double n = hvylya::core::sqr((center + diff) / (center - diff));
            return base_gain_ * n / (n + 1.0);
        }
        else
        {
            return base_gain_ / 2.0;
        }
    }

    virtual double errorWeight(double /*arg*/) const override
    {
        return 1.0;
    }

  private:
    double min_freq_, max_freq_, base_gain_;
};

} // anonymous namespace

int main(int /* argc */, char* /*argv*/[])
{
    const double SamplingFreq = 1000000.0;
    const double LeftFreq = (57000.0 - 7500.0) / SamplingFreq * 2 * M_PI;
    const double RightFreq = (57000.0 + 7500.0) / SamplingFreq * 2 * M_PI;
    const double BaseGain = 2.0;

    PmOptimizer<double> opt(1000, 1e-12);

    std::vector<const IFirBand<double>*> bands;

    //ConstFirBand<double> left_band(0.0, LeftFreq, BaseGain, 0.1);
    //ConstFirBand<double> left_band(0.0, 1000.0 / SamplingFreq * M_PI, 0, 0.1);
    ConstFirBand<double> left_band(0.0, LeftFreq - 1000.0 / SamplingFreq * 2 * M_PI, 0.0, 0.01);
    bands.push_back(&left_band);
    DecayedFirBand decayed_band(LeftFreq, RightFreq, BaseGain);
    bands.push_back(&decayed_band);
    //ConstFirBand<double> right_band(RightFreq, M_PI, BaseGain, 0.1);
    //ConstFirBand<double> right_band((SamplingFreq - 1000.0) / SamplingFreq * M_PI, M_PI, 0, 0.1);
    ConstFirBand<double> right_band(RightFreq + 1000.0 / SamplingFreq * 2 * M_PI, M_PI, 0, 0.01);
    bands.push_back(&right_band);

    //for (std::size_t i = 0; i < 25; ++i)
    //{
    //    double arg = i / 25.0;
    //    std::cout << "Left band(" << arg << ") = " << left_band.targetGain(arg) << std::endl;
    //}

    //for (std::size_t i = 0; i < 25; ++i)
    //{
    //    double arg = i / 25.0;
    //    std::cout << "Decayed band(" << arg << ") = " << decayed_band.targetGain(arg) << std::endl;
    //}

    //for (std::size_t i = 0; i < 25; ++i)
    //{
    //    double arg = i / 25.0;
    //    std::cout << "Right band(" << arg << ") = " << right_band.targetGain(arg) << std::endl;
    //}

    std::vector<double> taps;
    PmResult<double> result = opt.findTaps(taps, TapsCount, bands);

    std::cout << "Status = " << int(result.status) << ", iterations = " << result.iterations << ", max error = " << result.max_error << std::endl;
    std::cout << "Taps count = " << TapsCount << std::endl;
    for (std::size_t i = 0; i < taps.size(); ++i)
    {
        std::cout << taps[i] << std::endl;
    }

    return 0;
}
