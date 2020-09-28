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

#include <hvylya/filters/fm/cma_equalizer.h>

#ifdef DUMP_CMA_EQUALIZER_STATS
std::size_t g_samples = 0, g_taps_updates = 0, g_gain_updates = 0;
#endif // DUMP_CMA_EQUALIZER_STATS

using namespace hvylya::filters;

namespace {

const double MinAbsValue = 1e-20;

const double MinNormWeight = 1e-20;
const double MaxNormWeight = 1e-2;
const double NormWeightStep = 1e-2;

const double NormWeights[3] =
{
    1.0,
    0.5,
    0.25
};

const double MinTapReg = 1e-5;
const double MaxTapReg = 1e-2;
const double TapRegStep = 1e-3;

const double TapSumCoef = 1.0 / 250.0;

const double GainUpdateAccuracy = 1e-10;
const double TapsUpdateAccuracy = 1e-10;

const double GainRootAccuracy = 1e-8;
const std::size_t GainMaxRootIterations = 5;

const double Decay = 0.99999;
// Corresponds to 10K samples, as (1 - 0.99999^10000) / (1 - 0.99999) = 9516.3
const double MinWeight = 9516.0;

const std::size_t MinTapsUpdateInterval = 10;
const std::size_t MaxTapsUpdateInterval = 1000;

const std::size_t MinGainUpdateInterval = 10;
const std::size_t MaxGainUpdateInterval = 100;

const std::size_t DisabledTapsUpdateInterval = 100;

const std::size_t TapsFreqDivider = 50;

const std::size_t MinChannels = 2;
const std::size_t MaxChannels = 8;

}

template <typename T>
FmCmaEqualizer<T>::FmCmaEqualizer(std::size_t taps_count):
    taps_count_(taps_count),
    taps_count_mask_(taps_count - 1),
    taps_update_freq_(MinTapsUpdateInterval),
    gain_update_freq_(MinGainUpdateInterval),
    accum_update_freq_(1),
    left_until_taps_updates_(MinTapsUpdateInterval - 1),
    left_until_gain_updates_(0),
    left_until_disabled_taps_updates_(0),
    left_until_accum_updates_(accum_update_freq_ - 1),
    cur_output_index_(0),
    cur_weight_(T(0)),
    norm_weight_(T(1e-5)),
    tap_reg_(T(1e-3)),
    min_samples_accumulated_(false)
{
    CHECK_GT(taps_count, 0) << "Taps count must be positive!";
    CHECK_EQ(taps_count % ComplexVector::Elements, 0) << "Taps count must be multiple of ComplexVector::Elements!";

    taps_.resize(taps_count_, ValueType(0));
    taps_indices_.resize(taps_count_, 0);
    taps_enabled_.resize(taps_count_, false);
    outputs_.resize(2 * taps_count_, ValueType(0));
    adjusted_taps_.resize(taps_count_, ValueType(0));
    a_.resize(taps_count_, ValueType(0));
    b_.resize(taps_count_, ValueType(0));
    a_interm_.resize(taps_count_, ValueType(0));
    b_interm_.resize(taps_count_, ValueType(0));
    taps_[0] = T(1);
    gain_ = T(1);
    tap0_mag2_ = T(1);
    taps_enabled_[0] = true;
    taps_enabled_count_ = 1;
    adjusted_taps_[0] = T(1);
    accumulated_samples_ = 0;

    decays_.resize(MaxTapsUpdateInterval + 1);
    weight_updates_.resize(MaxTapsUpdateInterval + 1);

    decays_[0] = 1;
    weight_updates_[0] = 0;

    const AccScalarType decay = AccScalarType(Decay);
    for (std::size_t i = 1; i <= MaxTapsUpdateInterval; ++i)
    {
        decays_[i] = decays_[i - 1] * decay;
        weight_updates_[i] = (1.0 - decays_[i]) / (1.0 - decay);
    }
}

template <typename T>
void FmCmaEqualizer<T>::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    const auto& input_data = std::get<0>(input);
    auto& output_data = std::get<0>(output);

    const std::size_t data_size = std::min(input_data.size(), output_data.size());

    const ValueType* input_ptr = &input_data[0];
    std::size_t input_offset = 0;
    core::alignPointer(input_ptr, input_offset);

    // This loop is pretty tight, moving 'input_offset' addition to loop bounds instead of performing it
    // every cycle results in better performance.
    const std::size_t input_data_size_offsetted = data_size + input_offset;
    std::size_t input_index = input_offset, output_index = 0;
    const std::size_t taps_count = taps_count_, taps_count_mask = taps_count_mask_;

    for (; input_index < input_data_size_offsetted; ++input_index, ++output_index)
    {
        ValueType sample(input_ptr[input_index]);
        ValueType new_output = getOutput(sample);

        outputs_[cur_output_index_] = new_output;
        outputs_[cur_output_index_ + taps_count] = new_output;
        output_data[output_index] = new_output;

        updateIntermAccumulators(sample, new_output);

#ifdef DUMP_CMA_EQUALIZER_STATS
        ++g_samples;
#endif // DUMP_CMA_EQUALIZER_STATS

        if (!left_until_taps_updates_)
        {
#ifdef DUMP_CMA_EQUALIZER_STATS
            ++g_taps_updates;
#endif // DUMP_CMA_EQUALIZER_STATS
            updateAccumulators();

            if (min_samples_accumulated_)
            {
                updateNormWeight();

                if (!left_until_gain_updates_)
                {
#ifdef DUMP_CMA_EQUALIZER_STATS
                        ++g_gain_updates;
#endif // DUMP_CMA_EQUALIZER_STATS
                    ValueType prev_gain = gain_;
                    updateGain();
                    ValueType gain_diff = prev_gain - gain_;

                    if (std::real(gain_diff * std::conj(gain_diff)) < GainUpdateAccuracy)
                    {
                        gain_update_freq_ = std::min(MaxGainUpdateInterval, gain_update_freq_ + 1);
                    }
                    else
                    {
                        gain_update_freq_ = std::max(MinGainUpdateInterval, gain_update_freq_ - 1);
                    }

                    left_until_gain_updates_ = gain_update_freq_ - 1;
                }
                else
                {
                    --left_until_gain_updates_;
                }

                last_taps_diff_ = 0;
                T tap_mag_sum = updateTaps(!left_until_disabled_taps_updates_);
                updateTapReg(tap_mag_sum);

                if (last_taps_diff_ < TapsUpdateAccuracy)
                {
                    taps_update_freq_ = std::min(MaxTapsUpdateInterval, taps_update_freq_ + 1);
                }
                else
                {
                    taps_update_freq_ = std::max(MinTapsUpdateInterval, taps_update_freq_ - 1);
                }

                accum_update_freq_ = std::max<std::size_t>(1, taps_update_freq_ / TapsFreqDivider);
                left_until_accum_updates_ = accum_update_freq_ - 1;

                if (left_until_disabled_taps_updates_)
                {
                    --left_until_disabled_taps_updates_;
                }
                else
                {
                    left_until_disabled_taps_updates_ = DisabledTapsUpdateInterval - 1;
                }
            }
            else
            {
                if (cur_weight_ > MinWeight)
                {
                    norm_weight_ *= core::abs(core::SimdComplex<T>(ValueType(b_[0] / cur_weight_)))[0];
                    min_samples_accumulated_ = true;
                }
            }

            left_until_taps_updates_ = taps_update_freq_ - 1;

        }
        else
        {
            --left_until_taps_updates_;
        }

        cur_output_index_ = (cur_output_index_ + taps_count - 1) & taps_count_mask;
    }

    CHECK_EQ(input_index, input_data_size_offsetted);
    CHECK_EQ(output_index, data_size);

    input_data.advance(data_size);
    output_data.advance(data_size);
}

template <typename T>
typename FmCmaEqualizer<T>::ValueType
FmCmaEqualizer<T>::getOutput(ValueType sample)
{
    outputs_[cur_output_index_] = sample;

    core::SimdComplex<T> corrected(0);
    for (std::size_t i = 0; i < taps_enabled_count_; ++i)
    {
        corrected += core::SimdComplex<T>(outputs_[cur_output_index_ + taps_indices_[i]]) * core::SimdComplex<T>(adjusted_taps_[i]);
    }

    return core::normalized(corrected)[0];
}

template <typename T>
void FmCmaEqualizer<T>::updateIntermAccumulators(ValueType sample, ValueType new_output)
{
    if (!left_until_accum_updates_)
    {
        ComplexVector coef(std::conj(new_output));
        ComplexVector samples(sample);
        ScalarVector decays = T(Decay);
        const std::size_t taps_count = taps_count_;
        for (std::size_t i = 0; i < taps_count; i += ComplexVector::Elements)
        {
            ComplexVector out(reinterpret_cast<const ComplexVectorNonAligned*>(&outputs_[i + cur_output_index_])->elements_);
            ComplexVector coef_out(coef * out);
            ComplexVector out_sample = multiplyConjugated(samples, out);
            *reinterpret_cast<ComplexVector*>(&a_interm_[i]) = fusedMultiplyAdd(*reinterpret_cast<const ComplexVector*>(&a_interm_[i]), decays, coef_out);
            *reinterpret_cast<ComplexVector*>(&b_interm_[i]) = fusedMultiplyAdd(*reinterpret_cast<const ComplexVector*>(&b_interm_[i]), decays, out_sample);
        }
        ++accumulated_samples_;
        left_until_accum_updates_ = accum_update_freq_ - 1;
    }
    else
    {
        --left_until_accum_updates_;
    }
}

template <typename T>
void FmCmaEqualizer<T>::updateAccumulators()
{
    static_assert(sizeof(AccScalarType) >= sizeof(T), "Intermediate and accumulating floating-point types are incompatible");

    AccScalarType acc_decay = decays_[accumulated_samples_];
    AccScalarVector acc_decays(acc_decay);
    const std::size_t taps_count = taps_count_;
    for (std::size_t i = 0; i < taps_count; i += ComplexVector::Elements)
    {
        auto a_interm_array = core::castVector<AccComplexVector>(*reinterpret_cast<ComplexVector*>(&a_interm_[i]));
        auto b_interm_array = core::castVector<AccComplexVector>(*reinterpret_cast<ComplexVector*>(&b_interm_[i]));

        for (std::size_t j = 0; j < a_interm_array.size(); ++j)
        {
            *reinterpret_cast<AccComplexVector*>(&a_[i + j * AccComplexVector::Elements]) =
                fusedMultiplyAdd(*reinterpret_cast<const AccComplexVector*>(&a_[i + j * AccComplexVector::Elements]), acc_decays, a_interm_array[j]);
            *reinterpret_cast<AccComplexVector*>(&b_[i + j * AccComplexVector::Elements]) =
                fusedMultiplyAdd(*reinterpret_cast<const AccComplexVector*>(&b_[i + j * AccComplexVector::Elements]), acc_decays, b_interm_array[j]);
        }
    }

    std::fill(a_interm_.begin(), a_interm_.end(), ValueType(0));
    std::fill(b_interm_.begin(), b_interm_.end(), ValueType(0));

    cur_weight_ = acc_decay * cur_weight_ + weight_updates_[accumulated_samples_];
    accumulated_samples_ = 0;
}

template <typename T>
void FmCmaEqualizer<T>::updateNormWeight()
{
    // Imaginary component of 0th tap is forced towards zero by normalization component,
    // so assume for simplicity that we can neglect it and deal only with real parts of adjacent taps.
    T taps_dir(0);
    for (std::size_t i = 0; i < sizeof(NormWeights) / sizeof(NormWeights[0]); ++i)
    {
        taps_dir += T(NormWeights[i]) * std::real(taps_[i + 1]);
    }
    norm_weight_ = std::max(T(MinNormWeight), std::min(T(MaxNormWeight), norm_weight_ * (T(1) + T(NormWeightStep) * std::max(T(-1), std::min(T(1), taps_dir)))));
}

template <typename T>
void FmCmaEqualizer<T>::updateGain()
{
    core::SimdComplex<T> sum((ValueType(b_[0])));
    for (std::size_t i = 1; i < taps_enabled_count_; ++i)
    {
        sum -= core::SimdComplex<T>(taps_[taps_indices_[i]]) * core::SimdComplex<T>(getCorrelation(0, taps_indices_[i]));
    }

    sum /= T(cur_weight_);
    T sum_mag = core::abs(sum)[0];

    T b = -sum_mag;
    T c = -norm_weight_;
    T d = -norm_weight_ * sum_mag;

    // norm_weight is typically quite small, so the solution is close to
    // sum_mag and, therefore, it's a good initial value for the Newton algorithm.
    T r = sum_mag, root_error = 0;
    std::size_t i = 0;
    do
    {
        T rr = r * r;
        T br = b * r;
        root_error = (rr + br + c) * r + d;
        r -= ((rr + br + c) * r + d) / std::max(T(MinAbsValue), T(3) * rr + T(2) * br + c);
    }
    while (std::abs(root_error) > GainRootAccuracy && ++i < GainMaxRootIterations);

    taps_[0] = (r / sum_mag * sum)[0];
    tap0_mag2_ = std::real(taps_[0] * std::conj(taps_[0]));
    gain_ = T(1) / taps_[0];
    adjusted_taps_[0] = gain_;
}

template <typename T>
T FmCmaEqualizer<T>::updateTaps(bool calc_disabled_taps)
{
    T tap_mag_sum(0);
    T tap_norm_inv = norm_weight_ / (tap0_mag2_ * core::sqr(T(cur_weight_)));
    ValueType tap_norm_inv2 = norm_weight_ / (std::conj(taps_[0]) * T(cur_weight_));
    T cur_weight_inv = T(1) / T(cur_weight_);

    std::size_t cur_enabled_tap_index = 1;
    const std::size_t taps_count = taps_count_;
    for (std::size_t i = 1; i < taps_count; ++i)
    {
        if (!taps_enabled_[i] && !calc_disabled_taps)
        {
            continue;
        }

        core::SimdComplex<T> tap_norm_right = ValueType(b_[0]);
        core::SimdComplex<T> tap_fit_right = core::SimdComplex<T>(ValueType(b_[i])) - core::SimdComplex<T>(taps_[0]) * core::SimdComplex<T>(getCorrelation(i, 0));
        for (std::size_t j = 1; j < taps_enabled_count_; ++j)
        {
            std::size_t tap_index = taps_indices_[j];
            if (tap_index != i)
            {
                core::SimdComplex<T> tap(taps_[tap_index]);
                tap_fit_right -= tap * core::SimdComplex<T>(getCorrelation(i, tap_index));
                tap_norm_right -= tap * core::SimdComplex<T>(getCorrelation(0, tap_index));
            }
        }

        core::SimdComplex<T> corr(getCorrelation(i, 0));

        T tap_norm_left = core::norm2(corr)[0] * tap_norm_inv;

        tap_norm_right *= tap_norm_inv;
        tap_norm_right -= tap_norm_inv2;
        tap_norm_right *= corr;

        T tap_fit_left = T(1);
        tap_fit_right *= cur_weight_inv;

        T tap_left = tap_norm_left + tap_fit_left;
        T tap_left_inv = T(1) / tap_left;
        core::SimdComplex<T> tap_right = tap_norm_right + tap_fit_right;

        core::SimdComplex<T> new_tap = tap_right * tap_left_inv;
        core::SimdScalar<T> new_tap_mag2 = core::norm2(new_tap);
        T new_tap_sqrt_inv = core::rsqrt(new_tap_mag2)[0];
        T new_tap_mag = new_tap_mag2[0] * new_tap_sqrt_inv;
        T reg = tap_reg_ * tap_left_inv;

        tap_mag_sum += new_tap_mag;

        if (new_tap_mag > reg)
        {
            core::SimdComplex<T> new_tap_corrected = new_tap * (T(1) - reg * new_tap_sqrt_inv);
            core::SimdComplex<T> tap_diff(new_tap_corrected - core::SimdComplex<T>(taps_[i]));
            last_taps_diff_ += core::norm2(tap_diff)[0];
            taps_[i] = new_tap_corrected[0];
            if (!taps_enabled_[i])
            {
                taps_enabled_[i] = true;
                ++taps_enabled_count_;
                std::copy_backward(&taps_indices_[cur_enabled_tap_index], &taps_indices_[taps_enabled_count_ - 1], &taps_indices_[taps_enabled_count_]);
            }
            taps_indices_[cur_enabled_tap_index] = i;
            adjusted_taps_[cur_enabled_tap_index] = (-new_tap_corrected * gain_)[0];
            ++cur_enabled_tap_index;
        }
        else
        {
            last_taps_diff_ += core::norm2(core::SimdComplex<T>(taps_[i]))[0];
            taps_[i] = T(0);
            if (taps_enabled_[i])
            {
                taps_enabled_[i] = false;
                --taps_enabled_count_;
                std::copy(&taps_indices_[cur_enabled_tap_index + 1], &taps_indices_[taps_enabled_count_ + 1], &taps_indices_[cur_enabled_tap_index]);
            }
        }
    }

    return tap_mag_sum;
}

template <typename T>
void FmCmaEqualizer<T>::updateTapReg(T tap_mag_sum)
{
    T step;

    if (taps_enabled_count_ < MinChannels)
    {
        step = -T(TapRegStep);
    }
    else if (taps_enabled_count_ > MaxChannels || taps_enabled_[1])
    {
        step = T(TapRegStep);
    }
    else
    {
        step = std::max(T(-TapRegStep), std::min(T(TapRegStep), (tap_mag_sum * T(TapSumCoef) - tap_reg_)));
    }

    tap_reg_ = std::max(T(MinTapReg), std::min(T(MaxTapReg), tap_reg_ * (T(1) + step)));
}

template <typename T>
typename FmCmaEqualizer<T>::ValueType
FmCmaEqualizer<T>::getCorrelation(std::size_t row, std::size_t col)
{
    return ValueType(col < row ? std::conj(a_[row - col]) : a_[col - row]);
}

template class hvylya::filters::FmCmaEqualizer<float>;
