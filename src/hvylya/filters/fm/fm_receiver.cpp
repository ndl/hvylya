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

#include <hvylya/filters/fm/fm_receiver.h>

#include <hvylya/filters/costas_loop.h>
#include <hvylya/filters/fft_filter.h>
#include <hvylya/filters/fir_filter.h>
#include <hvylya/filters/fm/cma_equalizer.h>
#include <hvylya/filters/fft_translating_filter.h>
#include <hvylya/filters/fir_translating_filter.h>
#include <hvylya/filters/mapper_filter.h>
#include <hvylya/filters/pll_generator.h>
#include <hvylya/filters/resampler.h>
#include <hvylya/filters/sampler.h>

#include <hvylya/filters/fm/fm_constants.h>
#include <hvylya/filters/fm/fm_decoder.h>
#include <hvylya/filters/fm/fm_pll_decoder.h>
#include <hvylya/filters/fm/fm_deemphasizer.h>
#include <hvylya/filters/fm/fm_snr_estimator.h>
#include <hvylya/filters/fm/fm_stereo_extractor.h>
#include <hvylya/filters/fm/fm_stereo_demultiplexer.h>

#include <hvylya/filters/fm/rds_bits_decoder.h>
#include <hvylya/filters/fm/rds_demodulator.h>
#include <hvylya/filters/fm/rds_groups_decoder.h>
#include <hvylya/filters/fm/rds_messages_decoder.h>

#include <hvylya/filters/connect.h>

// #define INJECTED_DEMOD_NOISE_STDDEV 0.01f
// #define DUMP_INTERMEDIATE_DATA
// #define USE_FM_PLL_DECODER

#ifdef INJECTED_DEMOD_NOISE_STDDEV
#   include <hvylya/filters/gaussian_noise_source.h>
#endif // INJECTED_DEMOD_NOISE_STDDEV

#if defined(DUMP_INTERMEDIATE_DATA) || defined(RDS_DUMP_BITS)
#   include <hvylya/filters/file_sink.h>
#endif // defined(DUMP_INTERMEDIATE_DATA) || defined(RDS_DUMP_BITS)

using namespace hvylya::core;
using namespace hvylya::filters;
using namespace hvylya::filters::fm;

namespace {

const std::size_t FmEqualizerTapsCount = 32;

const std::size_t NoiseSamplingRate = 5000;
const std::size_t NoiseSamplingRatio = IntermediateSamplingRate / NoiseSamplingRate;
const std::size_t NoiseAudioSamplingRatio = IntermediateAudioSamplingRate / NoiseSamplingRate;
// RdsSymbolRate is smaller than PilotStereoFrequency, so using it for the
// common window size calculation.
const std::size_t NoiseWindowSize = 2 * std::size_t(IntermediateSamplingRate / RdsSymbolRate);
const std::size_t SnrRate = 10;

const double PllLoopBandwidth = 0.001;

} // anonymous namespace

template <typename T>
struct FmReceiver<T>::FmReceiverImpl
{
#ifdef INJECTED_DEMOD_NOISE_STDDEV
    // These have to be declared first, as we're referencing them.
    static void noiseAdder(const T& in0, const T& in1, T& out)
    {
        out = in0 + in1;
    }
#endif // INJECTED_DEMOD_NOISE_STDDEV

    static void realExtractor(const std::complex<T>& in, T& out)
    {
        out = in.real();
    }

    static void pilotTrippler(const std::complex<T>& in, std::complex<T>& out)
    {
        out = in * in * in;
    }

    struct SnrRecorder
    {
        SnrRecorder(std::atomic<T>& snr): snr_(snr) { }

        void operator() (const T& snr) { snr_.store(snr); }

        std::atomic<T>& snr_;
    };

    // Decimators:
    FirFilter<T, T, FmDemodDecimatorTapsCount> decimator_interm_;
    FirFilter<T, T, StereoDecimatorTapsCount> audio_stereo_decimator_;

    // Other FIR FFT filters:
    FftFilter<std::complex<T>, T> fm_band_filter_;
    //FftTranslatingFilter<std::complex<T>, T> fm_band_filter_;
    FmCmaEqualizer<T> fm_equalizer_;
    FftFilter<T, std::complex<T>> stereo_pilot_filter_;
    FftFilter<T, T>
        audio_mono_decimator_,
        stereo_bandpass_filter_,
        rds_bandpass_filter_,
        rds_demodulated_filter_,
        rds_symbol_shape_filter_,
        noise_extractor_filter_;

    MapperFilter<decltype(&FmReceiverImpl::pilotTrippler)> pilot_trippler_;

    // The rest of noise-related filters:
    MapperFilter<decltype(&FmReceiverImpl::realExtractor)> real_extractor_;
    Sampler<T> noise_sampler_, pilot_sampler_, mono_sampler_, stereo_sampler_, rds_sampler_;
    FmSnrEstimator<T> pilot_snr_estimator_, mono_snr_estimator_, stereo_snr_estimator_, rds_snr_estimator_;

    MapperFilter<SnrRecorder> pilot_snr_recorder_, mono_snr_recorder_, stereo_snr_recorder_, rds_snr_recorder_;
    std::atomic<T> pilot_snr_, mono_snr_, stereo_snr_, rds_snr_;

#ifdef INJECTED_DEMOD_NOISE_STDDEV
    GaussianNoiseSource<T> noise_source_;
    MapperFilter<decltype(&FmReceiverImpl::noiseAdder)> noise_adder_;
#endif // INJECTED_DEMOD_NOISE_STDDEV

#ifdef DUMP_INTERMEDIATE_DATA
    FileSink<std::int8_t> rds_bits_sink_;
#endif // DUMP_INTERMEDIATE_DATA

#ifdef RDS_DUMP_BITS
    FileSink<T> rds_interm_bits_sink_;
#endif // RDS_DUMP_BITS

    // The rest of FM-related filters:
#ifdef USE_FM_PLL_DECODER
    FmPllDecoder<T> fm_decoder_;
#else
    FmDecoder<T> fm_decoder_;
#endif
    PllGenerator<T> pll_generator_;
    CostasLoop<T> costas_loop_;
    FmStereoExtractor<T> fm_stereo_extractor_;
    FmStereoDemultiplexer<T> fm_stereo_demultiplexer_;
    FmDeemphasizer<T> deemphasizer_left_, deemphasizer_right_;
    Resampler<T, T, AudioResamplerInterpolationRatio, AudioResamplerDecimationRatio, AudioResamplerTapsCount>
        resampler_left_,
        resampler_right_;

    // The rest of RDS filters:
    RdsDemodulator<T> rds_demodulator_;
    RdsBitsDecoder<T> rds_bits_decoder_;
    RdsGroupsDecoder rds_groups_decoder_;
    RdsMessagesDecoder rds_messages_decoder_;

    FmReceiverImpl():
        // Decimating FIR filter evaluates only every IntermediateDecimationRatio sample, whereas FftFilter would still
        // evaluate every sample - so FIR filter is faster for this particular taps configuration.
        decimator_interm_(FmDemodDecimatorTaps, false, IntermediateDecimationRatio),
        audio_stereo_decimator_(StereoDecimatorTaps, true, AudioDecimationRatio),

        //fm_band_filter_(FmBasebandTaps, FmBasebandTapsCount, T(-0.0361826 / 2 / M_PI / 2), true),
        fm_band_filter_(FmBasebandTaps, FmBasebandTapsCount, true),
        fm_equalizer_(FmEqualizerTapsCount),
        stereo_pilot_filter_(StereoPilotBandpassTaps, StereoPilotBandpassTapsCount, true),
        audio_mono_decimator_(MonoDecimatorTaps, MonoDecimatorTapsCount, true, AudioDecimationRatio),
        stereo_bandpass_filter_(StereoBandpassTaps, StereoBandpassTapsCount, true),
        rds_bandpass_filter_(RdsBandpassTaps, RdsBandpassTapsCount, true),
        rds_demodulated_filter_(RdsDemodulatedTaps, RdsDemodulatedTapsCount, true),
        // Don't use compensated delays for symbol shape matching, as it's supposed to match at the end of the symbol.
        rds_symbol_shape_filter_(RdsSymbolShapeTaps, RdsSymbolShapeTapsCount),
        noise_extractor_filter_(NoiseExtractorTaps, NoiseExtractorTapsCount, true),

        pilot_trippler_(&FmReceiverImpl::pilotTrippler),

        // Samplers for SNR estimation.
        real_extractor_(&FmReceiverImpl::realExtractor),
        noise_sampler_(NoiseSamplingRatio, NoiseWindowSize),
        pilot_sampler_(NoiseSamplingRatio, NoiseWindowSize),
        mono_sampler_(NoiseAudioSamplingRatio, NoiseWindowSize),
        stereo_sampler_(NoiseSamplingRatio, NoiseWindowSize),
        rds_sampler_(NoiseSamplingRatio, NoiseWindowSize),

        // SNR estimators.
        pilot_snr_estimator_(StereoPilotFrequency, StereoPilotBandwidth, StereoPilotStopband, NoiseExtractorFrequency, NoiseExtractorBandwidth, NoiseExtractorStopband, NoiseSamplingRate, SnrRate),
        mono_snr_estimator_(T(0), AudioBandwidth, AudioStopband, NoiseExtractorFrequency, NoiseExtractorBandwidth, NoiseExtractorStopband, NoiseSamplingRate, SnrRate),
        stereo_snr_estimator_(2 * StereoPilotFrequency, AudioBandwidth, AudioStopband, NoiseExtractorFrequency, NoiseExtractorBandwidth, NoiseExtractorStopband, NoiseSamplingRate, SnrRate),
        rds_snr_estimator_(3 * StereoPilotFrequency, RdsBandwidth, RdsStopband, NoiseExtractorFrequency, NoiseExtractorBandwidth, NoiseExtractorStopband, NoiseSamplingRate, SnrRate),

        // SNR recorders.
        pilot_snr_recorder_(SnrRecorder(pilot_snr_)),
        mono_snr_recorder_(SnrRecorder(mono_snr_)),
        stereo_snr_recorder_(SnrRecorder(stereo_snr_)),
        rds_snr_recorder_(SnrRecorder(rds_snr_)),

#ifdef INJECTED_DEMOD_NOISE_STDDEV
        noise_source_(0.0f, INJECTED_DEMOD_NOISE_STDDEV),
        noise_adder_(&FmReceiverImpl::noiseAdder),
#endif // INJECTED_DEMOD_NOISE_STDDEV

#ifdef DUMP_INTERMEDIATE_DATA
        rds_bits_sink_("/usr/src/carpc.extra/samples/hvylya/rds_bits_output-u8x1@1187.5.bin"),
#endif // DUMP_INTERMEDIATE_DATA

#ifdef RDS_DUMP_BITS
        rds_interm_bits_sink_("/usr/src/carpc.extra/samples/hvylya/rds_interm_bits_output-f32x1@250000.bin"),
#endif // RDS_DUMP_BITS

        fm_decoder_(InputSamplingRate),
        pll_generator_(
            float(PllLoopBandwidth),
            float(2 * M_PI * (StereoPilotFrequency - StereoPilotBandwidth) / IntermediateSamplingRate),
            float(2 * M_PI * (StereoPilotFrequency + StereoPilotBandwidth) / IntermediateSamplingRate)
        ),
        costas_loop_(T(0.005 * 2 * M_PI), T(0.5)),
        deemphasizer_left_(IntermediateAudioSamplingRate),
        deemphasizer_right_(IntermediateAudioSamplingRate),
        resampler_left_(AudioResamplerTaps),
        resampler_right_(AudioResamplerTaps)
    {
        connect(fm_band_filter_, fm_equalizer_, fm_decoder_, decimator_interm_);

#ifdef INJECTED_DEMOD_NOISE_STDDEV
        connect(decimator_interm_, makeChannel<0>(noise_adder_));
        connect(noise_source_, makeChannel<1>(noise_adder_));

        connect(noise_adder_, audio_mono_decimator_);
        connect(noise_adder_, stereo_pilot_filter_);
        connect(noise_adder_, stereo_bandpass_filter_);
        connect(noise_adder_, rds_bandpass_filter_);
        connect(noise_adder_, noise_extractor_filter_);
#else // INJECTED_DEMOD_NOISE_STDDEV
        connect(decimator_interm_, audio_mono_decimator_);
        connect(decimator_interm_, stereo_pilot_filter_);
        connect(decimator_interm_, stereo_bandpass_filter_);
        connect(decimator_interm_, rds_bandpass_filter_);
        connect(decimator_interm_, noise_extractor_filter_);
#endif // INJECTED_DEMOD_NOISE_STDDEV

#ifdef DUMP_INTERMEDIATE_DATA
        connect(rds_bits_decoder_, rds_bits_sink_);
#endif // DUMP_INTERMEDIATE_DATA

#ifdef RDS_DUMP_BITS
        connect(makeChannel<1>(rds_bits_decoder_), rds_interm_bits_sink_);
#endif // RDS_DUMP_BITS

        connect(stereo_pilot_filter_, pll_generator_);

        connect(pll_generator_, makeChannel<0>(fm_stereo_extractor_));
        connect(stereo_bandpass_filter_, makeChannel<1>(fm_stereo_extractor_));

        connect(fm_stereo_extractor_, audio_stereo_decimator_);

        connect(audio_mono_decimator_, makeChannel<0>(fm_stereo_demultiplexer_));
        connect(audio_stereo_decimator_, makeChannel<1>(fm_stereo_demultiplexer_));

        connect(makeChannel<0>(fm_stereo_demultiplexer_), deemphasizer_left_);
        connect(makeChannel<1>(fm_stereo_demultiplexer_), deemphasizer_right_);

        connect(deemphasizer_left_, resampler_left_);
        connect(deemphasizer_right_, resampler_right_);

        connect(rds_bandpass_filter_, makeChannel<0>(rds_demodulator_));
        connect(pll_generator_, pilot_trippler_);

#ifdef ADJUST_RDS_CARRIER_PHASE
        connect(rds_bandpass_filter_, makeChannel<0>(costas_loop_));
        connect(pilot_trippler_, makeChannel<1>(costas_loop_));
        connect(costas_loop_, makeChannel<1>(rds_demodulator_));

#else
        connect(pilot_trippler_, makeChannel<1>(rds_demodulator_));
#endif

        connect(rds_demodulator_, rds_demodulated_filter_, rds_symbol_shape_filter_);

        connect(rds_symbol_shape_filter_, makeChannel<0>(rds_bits_decoder_));
        connect(pll_generator_, makeChannel<1>(rds_bits_decoder_));

        connect(rds_bits_decoder_, rds_groups_decoder_, rds_messages_decoder_);

        connect(noise_extractor_filter_, noise_sampler_);

        connect(stereo_pilot_filter_, real_extractor_, pilot_sampler_);
        connect(pilot_sampler_, makeChannel<0>(pilot_snr_estimator_));
        connect(noise_sampler_, makeChannel<1>(pilot_snr_estimator_));
        connect(pilot_snr_estimator_, pilot_snr_recorder_);

        connect(audio_mono_decimator_, mono_sampler_);
        connect(mono_sampler_, makeChannel<0>(mono_snr_estimator_));
        connect(noise_sampler_, makeChannel<1>(mono_snr_estimator_));
        connect(mono_snr_estimator_, mono_snr_recorder_);

        connect(stereo_bandpass_filter_, stereo_sampler_);
        connect(stereo_sampler_, makeChannel<0>(stereo_snr_estimator_));
        connect(noise_sampler_, makeChannel<1>(stereo_snr_estimator_));
        connect(stereo_snr_estimator_, stereo_snr_recorder_);

        connect(rds_bandpass_filter_, rds_sampler_);
        connect(rds_sampler_, makeChannel<0>(rds_snr_estimator_));
        connect(noise_sampler_, makeChannel<1>(rds_snr_estimator_));
        connect(rds_snr_estimator_, rds_snr_recorder_);
    }
};

template <typename T>
FmReceiver<T>::FmReceiver():
    pimpl_(std::make_unique<FmReceiverImpl>())
{
}

template <typename T>
FmReceiver<T>::~FmReceiver()
{
}

template <typename T>
Channel FmReceiver<T>::inputChannel(std::size_t input_index)
{
    CHECK_EQ(0, input_index);
    return Channel(std::ref(pimpl_->fm_band_filter_), 0);
}

template <typename T>
Channel FmReceiver<T>::outputChannel(std::size_t output_index)
{
    CHECK_LT(output_index, 2);
    return Channel(std::ref(output_index == 0 ? pimpl_->resampler_left_ : pimpl_->resampler_right_), 0);
}

template <typename T>
std::size_t FmReceiver<T>::inputSamplingRate() const
{
    return InputSamplingRate;
}

template <typename T>
std::size_t FmReceiver<T>::outputAudioSamplingRate() const
{
    return OutputAudioSamplingRate;
}

template <typename T>
void FmReceiver<T>::setStereoWeight(T stereo_weight)
{
    pimpl_->fm_stereo_demultiplexer_.setStereoWeight(stereo_weight);
}

template <typename T>
RdsState FmReceiver<T>::rdsState() const
{
    return std::move(pimpl_->rds_messages_decoder_.state());
}

template <typename T>
RdsDecodingStats FmReceiver<T>::rdsDecodingStats() const
{
    return pimpl_->rds_groups_decoder_.stats();
}

template <typename T>
T FmReceiver<T>::pilotSnr() const
{
    return pimpl_->pilot_snr_.load();
}

template <typename T>
T FmReceiver<T>::monoSnr() const
{
    return pimpl_->mono_snr_.load();
}

template <typename T>
T FmReceiver<T>::stereoSnr() const
{
    return pimpl_->stereo_snr_.load();
}

template <typename T>
T FmReceiver<T>::rdsSnr() const
{
    return pimpl_->rds_snr_.load();
}

template class hvylya::filters::fm::FmReceiver<float>;
