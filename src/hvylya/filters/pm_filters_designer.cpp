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

#include <hvylya/filters/pm_filters_designer.h>

#include <hvylya/filters/fir_bands_designer.h>
#include <hvylya/filters/pm_optimizer.h>

using namespace hvylya::core;
using namespace hvylya::filters;

namespace {

const std::size_t MinPmOptimizerIterations = 1000;

FirFilterType parityToType(FirTapsParity parity, bool anti_symmetric)
{
    if (anti_symmetric)
    {
        switch (parity)
        {
            case FirTapsParity::Odd:
                return FirFilterType(3);

            case FirTapsParity::Even:
                return FirFilterType(4);

            case FirTapsParity::Any:
                return FirFilterType::AntiSymmetric;
        }
    }
    else
    {
        switch (parity)
        {
            case FirTapsParity::Odd:
                return FirFilterType(1);

            case FirTapsParity::Even:
                return FirFilterType(2);

            case FirTapsParity::Any:
                return FirFilterType::Symmetric;
        }
    }
}

FirTapsParity typeToParity(FirFilterType type)
{
    switch (type)
    {
        case FirFilterType(1):
        case FirFilterType(3):
            return FirTapsParity::Odd;

        case FirFilterType(2):
        case FirFilterType(4):
            return FirTapsParity::Even;

        case FirFilterType::Symmetric:
        case FirFilterType::AntiSymmetric:
            return FirTapsParity::Any;
    }
}

bool typeToAntiSymmetry(FirFilterType type)
{
    switch (type)
    {
        case FirFilterType(1):
        case FirFilterType(2):
        case FirFilterType::Symmetric:
            return false;

        case FirFilterType(3):
        case FirFilterType(4):
        case FirFilterType::AntiSymmetric:
            return true;
    }
}

template <typename T>
T attenuationToDev(T attenuation_db)
{
    return std::pow(10, -attenuation_db / 20);
}

template <typename T>
T rippleToDev(T ripple_db)
{
    T r = std::pow(10, ripple_db / 20);
    return (r - 1) / (r + 1);
}

// See E.C. Ifeachor, B.W. Jervis, "Digital Signal Processing: A Practical Approach", 2nd ed, (7.14).
template <typename T>
std::size_t estimateLowpassOrder(T passband, T stopband, T passband_dev, T stopband_dev)
{
    const T a1 = 5.309e-3;
    const T a2 = 7.114e-2;
    const T a3 = -4.761e-1;
    const T a4 = -2.66e-3;
    const T a5 = -5.941e-1;
    const T a6 = -4.278e-1;

    const T b1 = -0.51244;
    const T b2 = -11.01217;

    T diff_freq = std::abs(passband - stopband);
    T diff_pass = std::log10(passband_dev);
    T diff_stop = std::log10(stopband_dev);

    T diff_inf =
        diff_stop * (a1 * diff_pass * diff_pass + a2 * diff_pass + a3) +
        (a4 * diff_pass * diff_pass + a5 * diff_pass + a6);
    T f = b1 * (diff_pass - diff_stop) + b2;

    return std::size_t(diff_inf / diff_freq + f * diff_freq + 1);
}

std::size_t ensureValidTapsCount(std::size_t taps_count, FirTapsParity parity, std::size_t alignment)
{
    switch (parity)
    {
        case FirTapsParity::Any:
            break;

        case FirTapsParity::Odd:
            taps_count |= 1;
            break;

        case FirTapsParity::Even:
            taps_count += taps_count & 1;
            break;
    }

    return roundUp(taps_count & ~std::size_t(1), alignment) + (taps_count & 1);
}

template <typename T, typename ErrorEvaluator>
bool createFilter(
    std::vector<T>& taps,
    std::size_t estimated_taps_count,
    const std::vector<const IFirBand<T>*>& bands_ptrs,
    ErrorEvaluator evaluator,
    FirFilterType type,
    std::size_t alignment
)
{
    // Hopefully estimation is not too far off,
    // so decrease taps number in relatively small steps.
    const std::size_t TapsStepDivisor = 10;

    // PmOptimizer needs at least 3 taps to do anything.
    const std::size_t MinTapsCount = 3;

    bool anti_symmetric = typeToAntiSymmetry(type);
    FirTapsParity parity = typeToParity(type);

    std::size_t taps_count = ensureValidTapsCount(std::max<std::size_t>(estimated_taps_count, MinTapsCount), parity, alignment);
    std::size_t min_change = std::max<std::size_t>(parity == FirTapsParity::Any ? 1 : 2, alignment);

    std::vector<T> result_taps;
    PmOptimizer<T> optimizer(std::max<std::size_t>(MinPmOptimizerIterations, taps_count));

    std::size_t bounds[2] = { 0, 0 }, last_taps_count = 0;
    while (last_taps_count != taps_count)
    {
        last_taps_count = taps_count;
        PmResult<T> result = optimizer.findTaps(result_taps, taps_count, bands_ptrs, anti_symmetric);

        if (result.status != PmStatus::Succeeded)
        {
            return false;
        }

        if (evaluator(result))
        {
            taps = result_taps;
            bounds[1] = taps_count;
            if (bounds[0])
            {
                taps_count = (bounds[0] + bounds[1]) / 2;
            }
            else
            {
                taps_count -= std::min(
                    taps_count - MinTapsCount,
                    std::max<std::size_t>(estimated_taps_count / TapsStepDivisor, min_change)
                );
            }
        }
        else
        {
            bounds[0] = taps_count;
            if (bounds[1])
            {
                taps_count = (bounds[0] + bounds[1]) / 2;
            }
            else
            {
                taps_count += std::max<std::size_t>(estimated_taps_count / TapsStepDivisor, min_change);
            }
        }
        taps_count = ensureValidTapsCount(taps_count, parity, alignment);
    }

    return true;
}

} // anonymous namespace

template <typename T>
bool PmFiltersDesigner<T>::createLowpassFilter(
    std::vector<T>& taps, 
    T passband,
    T stopband,
    T ripple_db,
    T attenuation_db,
    T gain,
    FirFilterType type,
    std::size_t alignment
)
{
    std::vector<ConstFirBand<T>> bands;
    std::vector<const IFirBand<T>*> bands_ptrs;
    FirBandsDesigner<T>::createLowpassBands(bands, bands_ptrs, passband, stopband, gain);

    T ripple_dev = rippleToDev(ripple_db);
    T attenuation_dev = attenuationToDev(attenuation_db);

    T max_dev = std::max(attenuation_dev, ripple_dev);

    bands[0].setErrorWeight(max_dev / ripple_dev);
    bands[1].setErrorWeight(max_dev / attenuation_dev);

    return createFilter(
        taps,
        estimateLowpassOrder(passband, stopband, ripple_dev, attenuation_dev),
        bands_ptrs,
        [ripple_dev, attenuation_dev, &bands](const auto& result)
        {
            return
                result.max_error / bands[0].errorWeight() <= ripple_dev &&
                result.max_error / bands[1].errorWeight() <= attenuation_dev;
        },
        type,
        alignment
    );
}

template <typename T>
bool PmFiltersDesigner<T>::createHighpassFilter(
    std::vector<T>& taps, 
    T stopband,
    T passband,
    T ripple_db,
    T attenuation_db,
    T gain,
    FirFilterType type,
    std::size_t alignment
)
{
    std::vector<ConstFirBand<T>> bands;
    std::vector<const IFirBand<T>*> bands_ptrs;
    FirBandsDesigner<T>::createHighpassBands(bands, bands_ptrs, stopband, passband, gain);

    T ripple_dev = rippleToDev(ripple_db);
    T attenuation_dev = attenuationToDev(attenuation_db);

    T max_dev = std::max(attenuation_dev, ripple_dev);

    bands[0].setErrorWeight(max_dev / attenuation_dev);
    bands[1].setErrorWeight(max_dev / ripple_dev);

    return createFilter(
        taps,
        estimateLowpassOrder(passband, stopband, ripple_dev, attenuation_dev),
        bands_ptrs,
        [ripple_dev, attenuation_dev, &bands](const auto& result)
        {
            return
                result.max_error / bands[0].errorWeight() <= attenuation_dev &&
                result.max_error / bands[1].errorWeight() <= ripple_dev;
        },
        type,
        alignment
    );
}

template <typename T>
bool PmFiltersDesigner<T>::createBandpassFilter(
    std::vector<T>& taps, 
    T left_stopband,
    T left_passband,
    T right_passband,
    T right_stopband,
    T ripple_db,
    T attenuation_db,
    T gain,
    FirFilterType type,
    std::size_t alignment
)
{
    std::vector<ConstFirBand<T>> bands;
    std::vector<const IFirBand<T>*> bands_ptrs;
    FirBandsDesigner<T>::createBandpassBands(bands, bands_ptrs, left_stopband, left_passband, right_passband, right_stopband, gain);

    T ripple_dev = rippleToDev(ripple_db);
    T attenuation_dev = attenuationToDev(attenuation_db);

    T max_dev = std::max(attenuation_dev, ripple_dev);

    bands[0].setErrorWeight(max_dev / attenuation_dev);
    bands[1].setErrorWeight(max_dev / ripple_dev);
    bands[2].setErrorWeight(max_dev / attenuation_dev);

    std::size_t left_estimated_taps_count = estimateLowpassOrder(left_passband, left_stopband, ripple_dev, attenuation_dev);
    std::size_t right_estimated_taps_count = estimateLowpassOrder(right_passband, right_stopband, ripple_dev, attenuation_dev);

    return createFilter(
        taps,
        std::max(left_estimated_taps_count, right_estimated_taps_count),
        bands_ptrs,
        [ripple_dev, attenuation_dev, &bands](const auto& result)
        {
            return
                result.max_error / bands[0].errorWeight() <= attenuation_dev &&
                result.max_error / bands[1].errorWeight() <= ripple_dev  &&
                result.max_error / bands[2].errorWeight() <= attenuation_dev;
        },
        type,
        alignment
    );
}

template <typename T>
bool PmFiltersDesigner<T>::createHilbertBandpassTransform(
    std::vector<T>& taps, 
    T left_passband,
    T right_passband,
    T ripple_db,
    T gain,
    std::size_t alignment
)
{
    std::vector<ConstFirBand<T>> bands;
    std::vector<const IFirBand<T>*> bands_ptrs;
    FirBandsDesigner<T>::createHilbertBandpassBands(bands, bands_ptrs, left_passband, right_passband, gain);

    T ripple_dev = rippleToDev(ripple_db);
    std::size_t left_estimated_taps_count = estimateLowpassOrder(left_passband, T(0), ripple_dev, ripple_dev);
    std::size_t right_estimated_taps_count = estimateLowpassOrder(right_passband, T(0.5), ripple_dev, ripple_dev);

    return createFilter(
        taps,
        std::max(left_estimated_taps_count, right_estimated_taps_count),
        bands_ptrs,
        [ripple_dev](const auto& result)
        {
            return result.max_error <= ripple_dev;
        },
        FirFilterType(3),
        alignment
    );
}

template <typename T>
bool PmFiltersDesigner<T>::createHilbertHighpassTransform(
    std::vector<T>& taps, 
    T passband,
    T ripple_db,
    T gain,
    std::size_t alignment
)
{
    std::vector<ConstFirBand<T>> bands;
    std::vector<const IFirBand<T>*> bands_ptrs;
    FirBandsDesigner<T>::createHilbertHighpassBands(bands, bands_ptrs, passband, gain);

    T ripple_dev = rippleToDev(ripple_db);

    return createFilter(
        taps,
        estimateLowpassOrder(passband, T(0), ripple_dev, ripple_dev),
        bands_ptrs,
        [ripple_dev](const auto& result)
        {
            return result.max_error <= ripple_dev;
        },
        FirFilterType(4),
        alignment
    );
}

template <typename T>
bool PmFiltersDesigner<T>::createDifferentiator(
    std::vector<T>& taps, 
    T passband,
    T stopband,
    T ripple_db,
    T attenuation_db,
    T gain,
    FirTapsParity parity,
    std::size_t alignment
)
{
    // Not sure what's the best way to select this, but this value seems to work fine over multiple
    // different input parameters.
    const T MaxErrorWeight = 1e8;

    CHECK(passband > 0);
    CHECK(stopband > passband);
    CHECK(stopband <= 0.5);

    T ripple_dev = rippleToDev(ripple_db);
    T attenuation_dev = attenuationToDev(attenuation_db);
    T max_dev = std::max(attenuation_dev, ripple_dev);
    T min_diff_error_weight = max_dev / ripple_dev;

    DifferentiatorFirBand<T> pass_band(2 * M_PI * passband, gain, min_diff_error_weight, MaxErrorWeight);
    ConstFirBand<T> stop_band(2 * M_PI * stopband, M_PI, 0, max_dev / attenuation_dev);

    std::vector<const IFirBand<T>*> bands_ptrs = { &pass_band, &stop_band };

    return createFilter(
        taps,
        estimateLowpassOrder(passband, stopband, ripple_dev, attenuation_dev),
        bands_ptrs,
        [ripple_dev, attenuation_dev, min_diff_error_weight, &stop_band](const auto& result)
        {
            return
                result.max_error / min_diff_error_weight <= ripple_dev &&
                result.max_error / stop_band.errorWeight() <= attenuation_dev;
        },
        parityToType(parity, true),
        alignment
    );
}

template class hvylya::filters::PmFiltersDesigner<double>;
template class hvylya::filters::PmFiltersDesigner<long double>;
