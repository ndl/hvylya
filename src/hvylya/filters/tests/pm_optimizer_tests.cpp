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
#include <hvylya/filters/fir_bands_designer.h>

#include <hvylya/filters/tests/test_utils.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::core;
using namespace hvylya::filters;
using namespace hvylya::filters::tests;

const double hvylya::filters::tests::TapsAccuracy = 1e-8;

TEST(PmOptimizer, SimpleLowPassOdd)
{
    std::vector<ConstFirBand<double>> bands;
    std::vector<const IFirBand<double>*> bands_ptrs;
    FirBandsDesigner<double>::createLowpassBands(bands, bands_ptrs, 0.125, 0.25);
 
    PmOptimizer<double> optimizer;
    std::vector<double> taps;
    PmResult<double> result = optimizer.findTaps(taps, 21, bands_ptrs);

    EXPECT_EQ(PmStatus::Succeeded, result.status);
    checkTaps({ -0.002855688710444133, -0.008091028416227743, 0.0007727404007510196, 0.01850653343409352, 0.02048512530427189, -0.01699727115556037, -0.06189399849320443, -0.03458277020811547, 0.1064308948371107, 0.2892851992076997, 0.3741218533230179, 0.2892851992076997, 0.1064308948371107, -0.03458277020811547, -0.06189399849320443, -0.01699727115556037, 0.02048512530427189, 0.01850653343409352, 0.0007727404007510196, -0.008091028416227743, -0.002855688710444133 }, taps);
}
TEST(PmOptimizer, SimpleLowPassEven)
{
    std::vector<ConstFirBand<double>> bands;
    std::vector<const IFirBand<double>*> bands_ptrs;
    FirBandsDesigner<double>::createLowpassBands(bands, bands_ptrs, 0.35, 0.45);

    PmOptimizer<double> optimizer;
    std::vector<double> taps;
    PmResult<double> result = optimizer.findTaps(taps, 12, bands_ptrs);

    EXPECT_EQ(PmStatus::Succeeded, result.status);
    checkTaps({ 0.0421880005451884, -0.04750665162754484, 0.04376837054200686, -0.0002533336455815919, -0.120082418864465, 0.602958382617428, 0.602958382617428, -0.120082418864465, -0.0002533336455815919, 0.04376837054200686, -0.04750665162754484, 0.0421880005451884 }, taps);
}

TEST(PmOptimizer, SimpleHilbertOdd)
{
    std::vector<ConstFirBand<double>> bands;
    std::vector<const IFirBand<double>*> bands_ptrs;
    FirBandsDesigner<double>::createHilbertBandpassBands(bands, bands_ptrs, 0.05, 0.45);

    PmOptimizer<double> optimizer;
    std::vector<double> taps;
    PmResult<double> result = optimizer.findTaps(taps, 15, bands_ptrs, true);

    EXPECT_EQ(PmStatus::Succeeded, result.status);
    checkTaps({ -0.05298289898525856, -1.213843840256838e-15, -0.0881992625693174, -9.177843670234628e-16, -0.1868372720856577, -6.143234069592533e-16, -0.6278269514676499, 5.921827098222459e-17, 0.62782695146765, 7.327471962526033e-16, 0.1868372720856578, 1.036208156316813e-15, 0.08819926256931752, 1.332267629550188e-15, 0.05298289898525867 }, taps);
}

TEST(PmOptimizer, SimpleHilbertEven)
{
    std::vector<ConstFirBand<double>> bands;
    std::vector<const IFirBand<double>*> bands_ptrs;
    FirBandsDesigner<double>::createHilbertHighpassBands(bands, bands_ptrs, 0.05);

    PmOptimizer<double> optimizer;
    std::vector<double> taps;
    PmResult<double> result = optimizer.findTaps(taps, 16, bands_ptrs, true);

    EXPECT_EQ(PmStatus::Succeeded, result.status);
    checkTaps({ -0.03192808116863452, -0.0260443384763602, -0.03731952800333062, -0.05309518095919681, -0.07668819296174528, -0.1168460911990959, -0.2057956612156301, -0.6344631701728916, 0.6344631701728916, 0.2057956612156301, 0.1168460911990959, 0.07668819296174528, 0.05309518095919681, 0.03731952800333062, 0.0260443384763602, 0.03192808116863452 }, taps);
}

TEST(PmOptimizer, LowPassLarge)
{
    std::vector<ConstFirBand<double>> bands;
    std::vector<const IFirBand<double>*> bands_ptrs;
    FirBandsDesigner<double>::createLowpassBands(bands, bands_ptrs, 0.05, 0.055);

    PmOptimizer<double> optimizer;
    std::vector<double> taps;
    PmResult<double> result = optimizer.findTaps(taps, 1001, bands_ptrs);

    EXPECT_EQ(PmStatus::Succeeded, result.status);
    EXPECT_EQ(11, result.iterations);
    EXPECT_NEAR(5.30183e-05, result.delta, 1e-8);
    EXPECT_NEAR(5.30183e-05, result.max_error, 1e-8);
}

TEST(PmOptimizer, BandPassLarge)
{
    std::vector<ConstFirBand<double>> bands;
    std::vector<const IFirBand<double>*> bands_ptrs;
    FirBandsDesigner<double>::createBandpassBands(
        bands,
        bands_ptrs,
        (57000.0 - 4000.0) / 250000.0,
        (57000.0 - 2400) / 250000.0,
        (57000.0 + 2400) / 250000.0,
        (57000.0 + 4000.0) / 250000.0
    );

    PmOptimizer<double> optimizer;
    std::vector<double> taps;
    PmResult<double> result = optimizer.findTaps(taps, 601, bands_ptrs);

    EXPECT_EQ(PmStatus::Succeeded, result.status);
    EXPECT_EQ(37, result.iterations);
    EXPECT_NEAR(0.000347974, result.delta, 1e-8);
    EXPECT_NEAR(0.000347974, result.max_error, 1e-8);
}
