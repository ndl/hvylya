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

//#define DEBUG_VERBOSE
//#define DUMP_POLYNOMES

#ifdef DUMP_POLYNOMES
#   include <fstream>
#endif

using namespace hvylya::core;
using namespace hvylya::filters;

namespace {

//
// Grid node definition and corresponding helper functions.
//

template <typename T>
struct Node
{
    T freq, cos_freq, target_gain, actual_gain, error, error_weight;
};

template <typename T>
T nodeError(const Node<T>& node)
{
    return node.error_weight * (node.target_gain - node.actual_gain);
}

template <typename T>
T interpolate(T arg, T value0, T value1)
{
    CHECK(arg >= 0 && arg <= 1);
    return (1 - arg) * value0 + arg * value1;
}

template <typename T>
T bandWidth(const IFirBand<T>& band)
{
    std::pair<T, T> freqs = band.frequencies();
    return freqs.second - freqs.first;
}

template <typename T>
T adjustmentForFilterType(T freq, std::size_t taps_count, bool anti_symmetric)
{
    CHECK(anti_symmetric || !(taps_count % 2));
    T adjustment = !anti_symmetric ? std::cos(freq / 2) : (taps_count % 2 ? std::sin(freq) : std::sin(freq / 2));
    // For the lack of better threshold, just use epsilon.
    return std::max(adjustment, std::numeric_limits<T>::epsilon());
}

#ifdef DEBUG_VERBOSE

template <typename T>
inline std::ostream& operator <<(std::ostream& os, const Node<T>& node)
{
    os << "freq = " << node.freq << ", target gain = " << node.target_gain <<
        ", actual gain = " << node.actual_gain << ", error = " << node.error <<
        ", error weight = " << node.error_weight;
    return os;
}

#endif

#ifdef DUMP_POLYNOMES

template <typename T>
void dumpPolynome(const LagrangeInterpolator<T>& interpolator, std::size_t taps_count, bool anti_symmetric, const char* file_name)
{
    const std::size_t SamplesCount = 10000;
    std::ofstream ofs(file_name);
    for (std::size_t i = 0; i <= SamplesCount; ++i)
    {
        T x = i / T(2 * SamplesCount);
        T adjustment = (anti_symmetric || !(taps_count % 2))? adjustmentForFilterType(2 * M_PI * x, taps_count, anti_symmetric) : 1;
        ofs << x << " " << adjustment * interpolator.evaluate(std::cos(2 * M_PI * x)) << std::endl;
    }
}

#endif

} // anonymous namespace

template <typename T>
struct hvylya::filters::PmOptimizer<T>::PmOptimizerImpl
{
    hvylya::core::LagrangeInterpolator<T> interpolator_;
    std::vector<Node<T>> grid_;
    std::vector<std::size_t> extremums_;
    std::size_t max_iterations_, grid_density_;
    T accuracy_;

    PmOptimizerImpl(std::size_t max_iterations, T accuracy, std::size_t grid_density):
        interpolator_(2), // Use interval capacity calculated as 4 / (b - a)
        max_iterations_(max_iterations),
        grid_density_(grid_density),
        accuracy_(accuracy)
    {
    }

    PmResult<T> findTaps(std::vector<T>& taps, std::size_t taps_count, const std::vector<const IFirBand<T>*>& bands, bool anti_symmetric);

    void initializeGrid(const std::vector<const IFirBand<T>*>& bands, std::size_t taps_count);

    void addNodeToGrid(T arg, const IFirBand<T>& band);

    void adjustGrid(std::size_t taps_count, bool anti_symmetric);

    void updateInterpolatorArguments();

    void updateInterpolatorValues(T delta);

    T calculateDelta() const;

    std::size_t calculateErrors();

    void updateAllExtremums();

    void updateEndExtremums();

    void replaceSingleExtremum(std::size_t new_index);

    void frequencyToImpulseResponse(std::vector<T>& taps, std::size_t taps_count, bool anti_symmetric);
};

template <typename T>
PmOptimizer<T>::PmOptimizer(std::size_t max_iterations, T accuracy, std::size_t grid_density):
    pimpl_(std::make_unique<PmOptimizerImpl>(max_iterations, accuracy, grid_density))
{
}

template <typename T>
PmOptimizer<T>::~PmOptimizer()
{
}

template <typename T>
PmResult<T> PmOptimizer<T>::findTaps(std::vector<T>& taps, std::size_t taps_count, const std::vector<const IFirBand<T>*>& bands, bool anti_symmetric)
{
    return pimpl_->findTaps(taps, taps_count, bands, anti_symmetric);
}

template <typename T>
PmResult<T> PmOptimizer<T>::PmOptimizerImpl::findTaps(std::vector<T>& taps, std::size_t taps_count, const std::vector<const IFirBand<T>*>& bands, bool anti_symmetric)
{
    initializeGrid(bands, (taps_count + 1) / 2 + (anti_symmetric && (taps_count % 2) ? 0 : 1));
    adjustGrid(taps_count, anti_symmetric);

    std::size_t iter = 0;
    T abs_delta = 0, max_error = 0;
    for (; iter < max_iterations_; ++iter)
    {
        // Delta calculation requires up-to-date interpolator arguments, so order is important.
        updateInterpolatorArguments();

        T delta = calculateDelta();
        abs_delta = std::abs(delta);

        updateInterpolatorValues(delta);

#ifdef DUMP_POLYNOMES
        if (iter == 0)
        {
            dumpPolynome(interpolator_, taps_count, anti_symmetric, "poly_orig.dat");
        }
#endif

        std::size_t max_error_index = calculateErrors();
        max_error = std::abs(grid_[max_error_index].error);

#ifdef DEBUG_VERBOSE
        LOG(INFO) << "Max error = " << max_error;
        LOG(INFO) << "Grid at iteration " << iter << " after values updates and errors calculation:";
        for (std::size_t i = 0; i < grid_.size(); ++i)
        {
            LOG(INFO) << "grid[" << i << "]: " << grid_[i];
        }
#endif

        if (std::abs(max_error - abs_delta) <= accuracy_)
        {
            // All errors at extremums are <= delta or max iterations reached, stop.
            break;
        }

        updateAllExtremums();
        // Should be no-op in most cases, but just to make sure max extremum
        // is inserted even if it was skipped by updateAllExtremums.
        replaceSingleExtremum(max_error_index);
    }

    // Make sure that if we exited the loop because the maximum number
    // of iterations was reached, the values are at least consistent.
    if (iter == max_iterations_ - 1)
    {
        updateInterpolatorArguments();
        T delta = calculateDelta();
        abs_delta = std::abs(delta);
        updateInterpolatorValues(delta);
    }

#ifdef DUMP_POLYNOMES
    dumpPolynome(interpolator_, taps_count, anti_symmetric, "poly_final.dat");
#endif

    frequencyToImpulseResponse(taps, taps_count, anti_symmetric);

    PmResult<T> result;
    result.status = iter < max_iterations_ ? PmStatus::Succeeded : PmStatus::IterationsLimitExceeded;
    result.iterations = iter;
    result.delta = abs_delta;
    result.max_error = max_error;
    return result;
}

template <typename T>
void PmOptimizer<T>::PmOptimizerImpl::initializeGrid(const std::vector<const IFirBand<T>*>& bands, std::size_t extremums_count)
{
    grid_.clear();

    std::size_t extra_nodes = 0;
    T total_bands_length = 0;
    for (auto band: bands)
    {
        T width = bandWidth(*band);
        total_bands_length += width;
        extra_nodes += (width > 0) ? 2 : 1;
    }

    std::size_t inband_nodes_count = (grid_density_ + 1) * (extremums_count - 1) + 1;
    grid_.reserve(inband_nodes_count + extra_nodes);

    T step = total_bands_length / (inband_nodes_count - 1);

    for (std::size_t band_index = 0; band_index < bands.size(); ++band_index)
    {
        const IFirBand<T>& band = *bands[band_index];
        T width = bandWidth(band);

        if (width > 0)
        {
            std::size_t nodes_count = std::size_t(width / step + 2);
            for (std::size_t i = 0; i <= nodes_count; ++i)
            {
                addNodeToGrid(T(i) / nodes_count, band);
            }
        }
        else
        {
            addNodeToGrid(0, band);
        }
    }

#ifdef DEBUG_VERBOSE
    LOG(INFO) << "Initial grid:";

    for (std::size_t i = 0; i < grid_.size(); ++i)
    {
        LOG(INFO) << "grid[" << i << "]: " << grid_[i];
    }

    LOG(INFO) << "Initial extremums:";
#endif

    extremums_.resize(extremums_count);
    for (std::size_t i = 0; i < extremums_count; ++i)
    {
        extremums_[i] = i * grid_.size() / extremums_count;

#ifdef DEBUG_VERBOSE
        LOG(INFO) << "node index = " << extremums_[i];
#endif
    }
}

template <typename T>
void PmOptimizer<T>::PmOptimizerImpl::addNodeToGrid(T arg, const IFirBand<T>& band)
{
    std::pair<T, T> freqs = band.frequencies();

    grid_.push_back(Node<T>());
    grid_.back().freq = interpolate(arg, freqs.first, freqs.second);
    grid_.back().cos_freq = std::cos(grid_.back().freq);
    grid_.back().target_gain = band.targetGain(arg);
    grid_.back().error_weight = band.errorWeight(arg);
}

template <typename T>
void PmOptimizer<T>::PmOptimizerImpl::adjustGrid(std::size_t taps_count, bool anti_symmetric)
{
    if (!anti_symmetric && (taps_count % 2))
    {
        return;
    }

    for (std::size_t grid_index = 0; grid_index < grid_.size(); ++grid_index)
    {
        T adjustment = adjustmentForFilterType(grid_[grid_index].freq, taps_count, anti_symmetric);
        grid_[grid_index].target_gain /= adjustment;
        grid_[grid_index].error_weight *= adjustment;
    }
}

template <typename T>
T PmOptimizer<T>::PmOptimizerImpl::calculateDelta() const
{
    // Unlike polynome interpolation, delta calculation requires us to use
    // the last point as well, so we need to adjust polynome coefficients.
    std::vector<T> delta_weights(interpolator_.weights().begin(), interpolator_.weights().end());
    delta_weights.push_back(1);

    for (std::size_t i = 0; i < extremums_.size() - 1; ++i)
    {
        delta_weights[i] *= 1 / (interpolator_.scale() * (grid_[extremums_[i]].cos_freq - grid_[extremums_.back()].cos_freq));
        delta_weights.back() *= interpolator_.scale() * (grid_[extremums_.back()].cos_freq - grid_[extremums_[i]].cos_freq);
#ifdef DEBUG_VERBOSE
        LOG(INFO) << "b[" << i << "] = " << delta_weights[i];
#endif
    }
    delta_weights.back() = 1 / delta_weights.back();
#ifdef DEBUG_VERBOSE
    LOG(INFO) << "b[" << extremums_.size() - 1 << "] = " << delta_weights.back();
#endif

    T num = 0, denom = 0;
    int sign = 1;
    for (std::size_t k = 0; k < extremums_.size(); ++k, sign = -sign)
    {
        num += delta_weights[k] * grid_[extremums_[k]].target_gain;
        denom += delta_weights[k] * sign / grid_[extremums_[k]].error_weight;
    }

    T delta = num / denom;

#ifdef DEBUG_VERBOSE
    LOG(INFO) << "Delta = " << delta;
#endif

    if (std::abs(delta) < std::numeric_limits<T>::epsilon())
    {
        // Calculations still might be noisy and unstable with delta
        // being this small, but at least for the common case
        // of gain = 1 we should be able to have correct signs of
        // the oscillations at the extremums.
        T adjusted_delta = core::sign(delta) * std::numeric_limits<T>::epsilon();
#ifdef DEBUG_VERBOSE
        LOG(WARNING) << "Delta had to be adjusted from " << delta << " to " << adjusted_delta;
#endif
        delta = adjusted_delta;
    }

    return delta;
}

template <typename T>
void PmOptimizer<T>::PmOptimizerImpl::updateInterpolatorArguments()
{
    // TODO: optimize!
    std::vector<T> args(extremums_.size() - 1);
    for (std::size_t i = 0; i < extremums_.size() - 1; ++i)
    {
        args[i] = grid_[extremums_[i]].cos_freq;
    }

    interpolator_.updateArguments(args);
}

template <typename T>
void PmOptimizer<T>::PmOptimizerImpl::updateInterpolatorValues(T delta)
{
#ifdef DEBUG_VERBOSE
    LOG(INFO) << "Polynome values:";
#endif
    int sign = 1;
    for (std::size_t i = 0; i < extremums_.size(); ++i, sign = -sign)
    {
        grid_[extremums_[i]].actual_gain = grid_[extremums_[i]].target_gain - sign * delta / grid_[extremums_[i]].error_weight;
#ifdef DEBUG_VERBOSE
        LOG(INFO) << "p[" << i << "] = " << grid_[extremums_[i]].actual_gain;
#endif
        if (i < extremums_.size() - 1)
        {
            interpolator_.updateValue(i, grid_[extremums_[i]].actual_gain);
        }

        //CHECK_NEAR(interpolator_.evaluate(grid_[extremums_[i]].cos_freq), grid_[extremums_[i]].actual_gain, accuracy_);
    }
}

template <typename T>
std::size_t PmOptimizer<T>::PmOptimizerImpl::calculateErrors()
{
    T max_error = 0;
    std::size_t max_error_index = 0;
    for (std::size_t i = 0; i < grid_.size(); ++i)
    {
        if (i != extremums_.back())
        {
            // Last node is special as it's not interpolated, so avoid overriding its value.
            grid_[i].actual_gain = interpolator_.evaluate(grid_[i].cos_freq);
        }

        T error = nodeError(grid_[i]), abs_error = std::abs(error);
        grid_[i].error = error;

        if (abs_error > max_error)
        {
            max_error = abs_error;
            max_error_index = i;
        }
    }
    return max_error_index;
}

template <typename T>
void PmOptimizer<T>::PmOptimizerImpl::updateAllExtremums()
{
    for (std::size_t ext_index = 0; ext_index < extremums_.size(); ++ext_index)
    {
        std::size_t grid_index = extremums_[ext_index];
        int sign = core::sign(grid_[grid_index].error);

        if (sign)
        {
            T max_error = std::abs(grid_[grid_index].error);
            std::size_t max_error_index = grid_index;

            for (
                std::size_t left_offset = 1;
                grid_index >= left_offset && core::sign(grid_[grid_index - left_offset].error) * sign > 0;
                ++left_offset
            )
            {
                T abs_error = std::abs(grid_[grid_index - left_offset].error);
                if (abs_error > max_error)
                {
                    max_error = abs_error;
                    max_error_index = grid_index - left_offset;
                }
            }

            for (
                std::size_t right_offset = 1;
                grid_index + right_offset < grid_.size() && core::sign(grid_[grid_index + right_offset].error) * sign > 0;
                ++right_offset
            )
            {
                T abs_error = std::abs(grid_[grid_index + right_offset].error);
                if (abs_error > max_error)
                {
                    max_error = abs_error;
                    max_error_index = grid_index + right_offset;
                }
            }

            extremums_[ext_index] = max_error_index;
        }
#ifdef DEBUG_VERBOSE
        else
        {
            LOG(WARNING) << "Detected zero error sign while updating extremums";
        }
#endif
    }

    updateEndExtremums();
}

template <typename T>
void PmOptimizer<T>::PmOptimizerImpl::updateEndExtremums()
{
    // Check if there's an error on the leftmost or rightmost side of the current extremums set
    // that has the opposite sign to the corresponding extremum and that must be a part of the
    // extremums set.

    T max_left_error = core::sign(grid_[extremums_.front()].error);
    std::size_t max_left_error_index = extremums_.front();
    int left_sign = core::sign(grid_[extremums_.front()].error);
    bool found_left = false;
    for (std::size_t i = 0; i < extremums_.front(); ++i)
    {
        T error = grid_[i].error;
        T abs_error = std::abs(error);
        if (abs_error > max_left_error && core::sign(error) != left_sign)
        {
            found_left = true;
            max_left_error = abs_error;
            max_left_error_index = i;
        }
    }

    T max_right_error = std::abs(grid_[extremums_.back()].error);
    std::size_t max_right_error_index = extremums_.back();
    int right_sign = core::sign(grid_[extremums_.back()].error);
    bool found_right = false;
    for (std::size_t i = extremums_.back(); i < grid_.size(); ++i)
    {
        T error = grid_[i].error;
        T abs_error = std::abs(error);
        if (abs_error > max_right_error && core::sign(error) != right_sign)
        {
            found_right = true;
            max_right_error = abs_error;
            max_right_error_index = i;
        }
    }

    // Of these two statements only one (or none) should be executed
    // due to the second condition.

    if (found_left && max_left_error > max_right_error)
    {
        replaceSingleExtremum(max_left_error_index);
    }

    if (found_right && max_right_error > max_left_error)
    {
        replaceSingleExtremum(max_right_error_index);
    }
}

template <typename T>
void PmOptimizer<T>::PmOptimizerImpl::replaceSingleExtremum(std::size_t new_index)
{
#ifdef DEBUG_VERBOSE
    LOG(INFO) << "Replacement node: " << grid_[new_index];
#endif

    int new_sign = core::sign(grid_[new_index].error);
    if (new_index < extremums_[0])
    {
        if (new_sign != core::sign(nodeError(grid_[extremums_[0]])))
        {
            std::copy_backward(extremums_.begin(), extremums_.end() - 1, extremums_.end());
        }
        extremums_[0] = new_index;
    }
    else if (new_index > extremums_.back())
    {
        if (new_sign != core::sign(nodeError(grid_[extremums_.back()])))
        {
            std::copy(extremums_.begin() + 1, extremums_.end(), extremums_.begin());
        }
        extremums_.back() = new_index;
    }
    else
    {
        std::size_t closest_ext_index = std::numeric_limits<std::size_t>::max();

        for (std::size_t i = 0; i < extremums_.size() - 1; ++i)
        {
            if (new_index >= extremums_[i] && new_index <= extremums_[i + 1])
            {
                closest_ext_index = (new_sign == core::sign(nodeError(grid_[extremums_[i]]))) ? i : i + 1;
                break;
            }
        }

        CHECK_NE(std::numeric_limits<std::size_t>::max(), closest_ext_index);

        extremums_[closest_ext_index] = new_index;
    }
}

template <typename T>
void PmOptimizer<T>::PmOptimizerImpl::frequencyToImpulseResponse(std::vector<T>& taps, std::size_t taps_count, bool anti_symmetric)
{
    bool type4 = anti_symmetric && !(taps_count % 2);

    std::size_t sums_count = (taps_count % 2) ? (taps_count - 1) / 2 : taps_count / 2 - 1;
    std::size_t freqs_count = type4 ? taps_count / 2 : sums_count;
    T offset = (taps_count - 1) / T(2);

    std::vector<T> freq_samples(freqs_count + 1);
    for (std::size_t i = 0; i <= freqs_count; ++i)
    {
        T freq = 2 * M_PI * i / taps_count;
        T adjustment = (anti_symmetric || !(taps_count % 2))? adjustmentForFilterType(freq, taps_count, anti_symmetric) : 1;
        freq_samples[i] = adjustment * interpolator_.evaluate(std::cos(freq));
    }

    taps.resize(taps_count);
    for (std::size_t tap_index = 0; tap_index < taps_count; ++tap_index)
    {
        T h =
            type4 ?
            freq_samples[freqs_count] * std::sin(M_PI * (tap_index - offset)) :
            freq_samples[0];

        T freq = 2 * M_PI * (tap_index - offset) / taps_count;

        for (std::size_t k = 1; k <= sums_count; ++k)
        {
            h += 2 * freq_samples[k] * (anti_symmetric ? std::sin(k * freq) : std::cos(k * freq));
        }

        taps[tap_index] = h / taps_count;
    }
}

template class hvylya::filters::PmOptimizer<double>;
template class hvylya::filters::PmOptimizer<long double>;
