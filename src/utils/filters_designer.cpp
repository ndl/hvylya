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
#include <hvylya/filters/fft_filter.h>

#include <iostream>
#include <fstream>
#include <iomanip>

// #define INTEGRATE_BANDWIDTH

using namespace hvylya::core;
using namespace hvylya::filters;

namespace {

const double Gain = 1.0;
const double Ripple = 0.01;
// FM baseband contains signals with very high magnitude difference,
// smaller ripple prevents small magnitude parts of the signal to be distorted.
const double FmBasebandRipple = 1e-4;
const double Attenuation = 60;

// Use AVX alignment.
const std::size_t DefaultAlignment = 8;
const std::size_t CompensatedDelayAlignment = 16;

const std::size_t InputSamplingRate = 1000000;
const std::size_t IntermediateSamplingRate = 250000;
const std::size_t IntermediateAudioSamplingRate = 50000;
const std::size_t OutputAudioSamplingRate = 48000;

const std::size_t IntermediateDecimationRatio = InputSamplingRate / IntermediateSamplingRate;
const std::size_t AudioDecimationRatio = IntermediateSamplingRate / IntermediateAudioSamplingRate;
const std::size_t AudioResamplerInterpolationRatio = 24;
const std::size_t AudioResamplerDecimationRatio = 25;

const double StereoPilotFrequency = 19000.0;
const double StereoCarrierFrequency = 2 * StereoPilotFrequency;
const double RdsCarrierFrequency = 3 * StereoPilotFrequency;
const double StereoPilotBandwidth = 200.0;
const double StereoPilotStopband = StereoPilotBandwidth + 2000.0;
const double AudioBandwidth = 15000.0;
const double AudioStopband = AudioBandwidth + 2000.0;
const double FmChannelBandwidth = 120000.0;
const double FmChannelStopband = 130000.0;
const double RdsBandwidth = 2400.0;
const double RdsStopband = RdsBandwidth + 1500.0;
const double RdsSymbolRate = 1187.5;

const double NoiseExtractorFrequency = 65000.0;
const double NoiseExtractorBandwidth = 2000.0;
const double NoiseExtractorStopband = NoiseExtractorBandwidth + 3000.0;

const double FmDemodBandwidth = NoiseExtractorFrequency + NoiseExtractorStopband;

const char* TapsNamespaces[] = { "hvylya", "filters", "fm", nullptr };
const char* CountType = "std::size_t";
const char* TapType = "float";

// According to EN50067, 1.7, eq.3, Ht(f) = cos(%pi*f*d/4) for f in [0, 2/d]
// IFFT: integrate(cos(%pi*f*d/4)*exp(2*%pi*%i*t*f), f, -2/d, 2/d) => 2*d * cos(4*%pi*t/d) / (%pi*d^2 - 64*%pi*t^2).
double rdsSymbolSinglePhase(double t)
{
    return 2.0 / RdsSymbolRate * std::cos((4.0 * M_PI * t) * RdsSymbolRate ) / (M_PI / (RdsSymbolRate * RdsSymbolRate) - 64.0 * M_PI * t * t);
}

// According to EN50067, 1.7, eq. 1, combined response is a difference between two deltas located d/2 apart.
double rdsSymbolCombinedPhase(double t)
{
    return rdsSymbolSinglePhase(t + 1.0 / (4.0 * RdsSymbolRate)) - rdsSymbolSinglePhase(t - 1.0 / (4.0 * RdsSymbolRate));
}

// Logical zero shape inverted for FIR compatibility.
void createRdsSymbolShape(std::vector<double>& taps, std::size_t taps_count, std::size_t sampling_rate)
{
    const double Offset = (taps_count - 1.0) / 2.0;

    taps.resize(taps_count);

    for (std::size_t i = 0; i < taps_count; ++i)
    {
        taps[i] = rdsSymbolCombinedPhase((i - Offset) / sampling_rate);
    }
}

class FilterWriter
{
  public:
    FilterWriter(const char* header_preamble, const char* source_preamble, const char* output_base_name, const char* namespaces[], const char* count_type, const char* tap_scalar_type):
        ofs_header_((output_base_name + std::string(".h")).c_str()),
        ofs_source_((output_base_name + std::string(".cpp")).c_str()),
        count_type_(count_type),
        tap_scalar_type_(tap_scalar_type)
    {
        for (std::size_t i = 0; namespaces[i]; ++i)
        {
            namespaces_.push_back(namespaces[i]);
        }

        std::ifstream ifs_header_preamble(header_preamble);
        std::ifstream ifs_source_preamble(source_preamble);

        char buffer[BUFSIZ];
        while (ifs_header_preamble)
        {
            ifs_header_preamble.read(buffer, BUFSIZ);
            ofs_header_.write(buffer, ifs_header_preamble.gcount());
        }

        while (ifs_source_preamble)
        {
            ifs_source_preamble.read(buffer, BUFSIZ);
            ofs_source_.write(buffer, ifs_source_preamble.gcount());
        }

        ns_prefix_ = namespaces_[0];
        for (std::size_t i = 1; i < namespaces_.size(); ++i)
        {
            ns_prefix_.append("::");
            ns_prefix_.append(namespaces_[i]);
        }

        for (std::size_t i = 0; i < namespaces_.size(); ++i)
        {
            ofs_header_ << "namespace " << namespaces_[i] << " {" << std::endl;
        }
        ofs_header_ << std::endl;
    }

    ~FilterWriter()
    {
        ofs_header_ << std::endl;
        for (std::size_t i = 0; i < namespaces_.size(); ++i)
        {
            ofs_header_ << "} // namespace " << namespaces_[namespaces_.size() - i - 1] << std::endl;
        }
    }

    template <typename T>
    void writeTaps(const char* name, const std::vector<T>& taps)
    {
        LOG(INFO) << "Writing " << taps.size() << " taps for filter " << name;

        std::string tap_type = tapType(taps[0]);

        ofs_header_ << "static const " << count_type_ << " " << name << "TapsCount = " << taps.size() << "; " << std::endl;
        ofs_header_ << "extern const " << tap_type << " " << name << "Taps[" << name << "TapsCount];" << std::endl;
        ofs_header_ << std::endl;

        ofs_source_ << "const " << tap_type << " " << ns_prefix_ << "::" << name << "Taps[" << name << "TapsCount] =" << std::endl;
        ofs_source_ << "{" << std::endl;

        for (std::size_t i = 0; i < taps.size() - 1; ++i)
        {
            ofs_source_ << "    " << std::setprecision(19);
            writeTap(ofs_source_, taps[i]);
            ofs_source_ << "," << std::endl;
        }

        ofs_source_ << "    ";
        writeTap(ofs_source_, taps.back());
        ofs_source_ << std::endl;
        ofs_source_ << "};" << std::endl;
        ofs_source_ << std::endl;
    }

    template <typename T>
    std::string tapType(std::complex<T>) const
    {
        return "std::complex<" + tap_scalar_type_ + ">";
    }

    template <typename T>
    std::string tapType(T) const
    {
        return tap_scalar_type_;
    }

    template <typename T>
    void writeTap(std::ostream& os, std::complex<T> tap) const
    {
       os << "{ " << tap_scalar_type_ << "(" << tap.real() << "), " << tap_scalar_type_ << "(" << tap.imag() << ") }";
    }

    template <typename T>
    void writeTap(std::ostream& os, T tap) const
    {
       os << tap_scalar_type_ << "(" << tap << ")";
    }

    void writeConst(const char* name, std::size_t value)
    {
        ofs_header_ << "static const std::size_t " << name << " = " << value << ";" << std::endl;
    }

    void writeConst(const char* name, double value)
    {
        ofs_header_ << "static const double " << name << " = " << value << ";" << std::endl;
    }

    void writeHeaderNewLine()
    {
        ofs_header_ << std::endl;
    }

  private:
    std::ofstream ofs_header_, ofs_source_;
    std::vector<std::string> namespaces_;
    std::string ns_prefix_, count_type_, tap_scalar_type_;
};

#ifdef INTEGRATE_BANDWIDTH
template <typename T>
T integrate(const std::vector<T>& taps, T min_freq = T(0.0), T max_freq = T(0.5))
{
    const std::size_t BufferSize = IntermediateSamplingRate;
    const double FreqStep = 0.001;
    AlignedVector<T> input_vector(BufferSize), output_vector(BufferSize);

    double result = 0.0;

    for (double freq = std::max<double>(FreqStep, min_freq); freq <= std::min<double>(0.5, max_freq); freq += FreqStep)
    {
        FftFilter<T, T> filter(&taps[0], taps.size());

        for (std::size_t i = 0; i < BufferSize; ++i)
        {
            input_vector[i] = T(std::sin(freq * i * 2 * M_PI));
        }

        Slice<T> input(&input_vector[0], BufferSize), output(&output_vector[0], BufferSize);

        auto input_tuple = std::make_tuple(std::cref(input));
        auto output_tuple = std::make_tuple(std::ref(output));

        filter.process(input_tuple, output_tuple);

        std::size_t advanced_size = output.advancedSize();

        double out_amps_sum = 0.0, in_amps_sum = 0.0;
        for (std::size_t i = 0; i < advanced_size; ++i)
        {
            double in_amp = T(std::sin(freq * 2 * M_PI * (i + (taps.size() - 1.0) / 2.0)));
            double out_amp = output_vector[i];
            in_amps_sum += std::abs(in_amp);
            out_amps_sum += std::abs(out_amp);
        }

        in_amps_sum /= advanced_size;
        out_amps_sum /= advanced_size;

        result += (out_amps_sum / in_amps_sum) * FreqStep;
    }

    return T(result);
}
#endif // INTEGRATE_BANDWIDTH

} // anonymous namespace

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: filter-designer <header-preamble-file> <source-preamble-file> <output-file-prefix>" << std::endl;
        return 1;
    }

    std::vector<double> taps;
    FilterWriter writer(argv[1], argv[2], argv[3], TapsNamespaces, CountType, TapType);

    writer.writeConst("InputSamplingRate", InputSamplingRate);
    writer.writeConst("IntermediateSamplingRate", IntermediateSamplingRate);
    writer.writeConst("IntermediateAudioSamplingRate", IntermediateAudioSamplingRate);
    writer.writeConst("OutputAudioSamplingRate", OutputAudioSamplingRate);
    writer.writeConst("RdsSymbolRate", RdsSymbolRate);

    writer.writeHeaderNewLine();

    writer.writeConst("IntermediateDecimationRatio", IntermediateDecimationRatio);
    writer.writeConst("AudioDecimationRatio", AudioDecimationRatio);
    writer.writeConst("AudioResamplerInterpolationRatio", AudioResamplerInterpolationRatio);
    writer.writeConst("AudioResamplerDecimationRatio", AudioResamplerDecimationRatio);

    writer.writeHeaderNewLine();

    writer.writeConst("StereoPilotFrequency", StereoPilotFrequency);
    writer.writeConst("NoiseExtractorFrequency", NoiseExtractorFrequency);

    writer.writeHeaderNewLine();

    writer.writeConst("FmChannelBandwidth", FmChannelBandwidth);
    writer.writeConst("FmChannelStopband", FmChannelStopband);
    writer.writeConst("StereoPilotBandwidth", StereoPilotBandwidth);
    writer.writeConst("StereoPilotStopband", StereoPilotStopband);
    writer.writeConst("AudioBandwidth", AudioBandwidth);
    writer.writeConst("AudioStopband", AudioStopband);
    writer.writeConst("RdsBandwidth", RdsBandwidth);
    writer.writeConst("RdsStopband", RdsStopband);
    writer.writeConst("NoiseExtractorBandwidth", NoiseExtractorBandwidth);
    writer.writeConst("NoiseExtractorStopband", NoiseExtractorStopband);

    writer.writeHeaderNewLine();

    CHECK(PmFiltersDesigner<double>::createLowpassFilter(
        taps,
        FmChannelBandwidth / InputSamplingRate,
        FmChannelStopband / InputSamplingRate,
        FmBasebandRipple,
        Attenuation,
        Gain,
        FirFilterType(1),
        DefaultAlignment
    ));
    writer.writeTaps("FmBaseband", taps);

    CHECK(PmFiltersDesigner<double>::createLowpassFilter(
        taps,
        FmDemodBandwidth / InputSamplingRate,
        // We'll have other filters separating
        // their corresponding ranges, so the only
        // requirement here is to avoid aliasing.
        (0.5 * IntermediateSamplingRate) / InputSamplingRate,
        Ripple,
        Attenuation,
        Gain,
        FirFilterType(1),
        DefaultAlignment
    ));
    writer.writeTaps("FmDemodDecimator", taps);

    std::vector<double> real_taps;
    CHECK(PmFiltersDesigner<double>::createBandpassFilter(
        real_taps,
        // Stopband is a compromise between excluding
        // as much noise as possible and keeping
        // the size of the filter manageable.
        // Hard limits are [15kHz, 23 kHz].
        (StereoPilotFrequency - StereoPilotStopband) / IntermediateSamplingRate,
        // Pilot is centered at 19 kHz. Although the
        // standard requires +/- 2Hz accuracy, allow
        // somewhat larger variation, mainly to
        // compensate for receiver imperfections.
        (StereoPilotFrequency - StereoPilotBandwidth) / IntermediateSamplingRate,
        (StereoPilotFrequency + StereoPilotBandwidth) / IntermediateSamplingRate,
        (StereoPilotFrequency + StereoPilotStopband) / IntermediateSamplingRate,
        Ripple,
        Attenuation,
        Gain,
        FirFilterType(1),
        // We'll use the output of this filter for
        // coherent demodulation, so compensated delay
        // is necessary.
        CompensatedDelayAlignment
    ));

    std::vector<double> imag_taps;
    // See comments above for bands and alignment explanation.
    //
    // Knowing the number of taps from the calculation above, we could have created it
    // directly without searching for the optimum number of taps, but we nonetheless
    // do the full search to double-check everything is correct (= that found taps
    // do satisfy error restrictions and have the same size as 'real' ones).
    CHECK(PmFiltersDesigner<double>::createBandpassFilter(
        imag_taps,
        (StereoPilotFrequency - StereoPilotStopband) / IntermediateSamplingRate,
        (StereoPilotFrequency - StereoPilotBandwidth) / IntermediateSamplingRate,
        (StereoPilotFrequency + StereoPilotBandwidth) / IntermediateSamplingRate,
        (StereoPilotFrequency + StereoPilotStopband) / IntermediateSamplingRate,
        Ripple,
        Attenuation,
        Gain,
        // Create anti-symmetric filter to have the required phase shift.
        FirFilterType(3),
        CompensatedDelayAlignment
    ));

    CHECK_EQ(real_taps.size(), imag_taps.size());

    std::vector<std::complex<double>> complex_taps(real_taps.size());
    for (std::size_t i = 0; i < real_taps.size(); ++i)
    {
        complex_taps[i] = std::complex<double>(real_taps[i], imag_taps[i]);
    }

    writer.writeTaps("StereoPilotBandpass", complex_taps);

#ifdef INTEGRATE_BANDWIDTH
    std::vector<float> real_taps2(real_taps.begin(), real_taps.end());
    LOG(ERROR) << "Stereo pilot integral = " << integrate(real_taps2) <<
        ", naive calculation = " <<
        (StereoPilotBandwidth + StereoPilotStopband) / IntermediateSamplingRate;
#endif // INTEGRATE_BANDWIDTH

    CHECK(PmFiltersDesigner<double>::createBandpassFilter(
        taps,
        // The hard limit is the pilot tone, but keep it tighter than that to minimize noise.
        (StereoCarrierFrequency - AudioStopband) / IntermediateSamplingRate,
        (StereoCarrierFrequency - AudioBandwidth) / IntermediateSamplingRate,
        (StereoCarrierFrequency + AudioBandwidth) / IntermediateSamplingRate,
        // We're overlapping with RDS bandwidth, so try to compromise between low overlap and taps count.
        (StereoCarrierFrequency + AudioStopband) / IntermediateSamplingRate,
        Ripple,
        Attenuation,
        Gain,
        FirFilterType(1),
        // We'll use the output of this filter for
        // coherent demodulation, so compensated delay
        // is necessary.
        CompensatedDelayAlignment
    ));
    writer.writeTaps("StereoBandpass", taps);

#ifdef INTEGRATE_BANDWIDTH
    std::vector<float> taps2(taps.begin(), taps.end());
    LOG(ERROR) << "StereoBandpass integral = " << integrate(taps2) <<
        ", naive calculation = " <<
        (AudioBandwidth + AudioStopband) / IntermediateSamplingRate;
#endif // INTEGRATE_BANDWIDTH

    CHECK(PmFiltersDesigner<double>::createLowpassFilter(
        taps,
        AudioBandwidth / IntermediateSamplingRate,
        // The hard limit is the pilot tone, but keep it tighter than that to minimize noise.
        AudioStopband / IntermediateSamplingRate,
        Ripple,
        Attenuation,
        Gain,
        FirFilterType(1),
        // We'll use the output of this filter to calculate
        // the stereo signal coming from another filter,
        // so compensated delay is necessary.
        CompensatedDelayAlignment
    ));
    writer.writeTaps("MonoDecimator", taps);

#ifdef INTEGRATE_BANDWIDTH
    taps2.assign(taps.begin(), taps.end());
    LOG(ERROR) << "MonoDecimator integral = " << integrate(taps2) <<
        ", naive calculation = " <<
        (AudioBandwidth + AudioStopband) / (2 * IntermediateSamplingRate);
#endif // INTEGRATE_BANDWIDTH

    CHECK(PmFiltersDesigner<double>::createLowpassFilter(
        taps,
        AudioBandwidth / IntermediateSamplingRate,
        // Prevent aliasing.
        (0.5 * IntermediateAudioSamplingRate) / IntermediateSamplingRate,
        Ripple,
        Attenuation,
        Gain,
        FirFilterType(1),
        // We'll use the output of this filter to calculate
        // the stereo signal coming from another filter,
        // so compensated delay is necessary.
        CompensatedDelayAlignment
    ));
    writer.writeTaps("StereoDecimator", taps);

    CHECK(PmFiltersDesigner<double>::createLowpassFilter(
        taps,
        AudioBandwidth / (IntermediateAudioSamplingRate * AudioResamplerInterpolationRatio),
        // Prevent aliasing.
        0.5 / AudioResamplerDecimationRatio,
        Ripple,
        Attenuation,
        Gain,
        FirFilterType(1),
        // This will run in parallel for both audio channels,
        // so it's fine to have delay given it will be the same for both.
        DefaultAlignment
    ));
    writer.writeTaps("AudioResampler", taps);

    CHECK(PmFiltersDesigner<double>::createBandpassFilter(
        taps,
        (RdsCarrierFrequency - RdsStopband) / IntermediateSamplingRate,
        (RdsCarrierFrequency - RdsBandwidth) / IntermediateSamplingRate,
        (RdsCarrierFrequency + RdsBandwidth) / IntermediateSamplingRate,
        (RdsCarrierFrequency + RdsStopband) / IntermediateSamplingRate,
        Ripple,
        Attenuation,
        Gain,
        FirFilterType(1),
        // We'll use the output of this filter for
        // coherent demodulation, so compensated delay
        // is necessary.
        CompensatedDelayAlignment
    ));
    writer.writeTaps("RdsBandpass", taps);

#ifdef INTEGRATE_BANDWIDTH
    taps2.assign(taps.begin(), taps.end());
    LOG(ERROR) << "RdsBandpass integral = " << integrate(taps2) <<
        ", naive calculation = " <<
        (RdsBandwidth + RdsStopband) / IntermediateSamplingRate;
#endif // INTEGRATE_BANDWIDTH

    CHECK(PmFiltersDesigner<double>::createLowpassFilter(
        taps,
        RdsBandwidth / IntermediateSamplingRate,
        // While we can go as far as 2 * 57 kHz,
        // keep stopband low to minimize noise.
        RdsStopband / IntermediateSamplingRate,
        Ripple,
        Attenuation,
        Gain,
        FirFilterType(1),
        // This will be used in RDS bits decoding and needs
        // to be coherent with pilot tone PLL,
        // so compensated delay is necessary.
        CompensatedDelayAlignment
    ));
    writer.writeTaps("RdsDemodulated", taps);

    createRdsSymbolShape(
        taps,
        // This number of taps ensures that filter delay is exactly one symbol length.
        std::size_t(2 * IntermediateSamplingRate / RdsSymbolRate + 1),
        IntermediateSamplingRate
    );
    writer.writeTaps("RdsSymbolShape", taps);

    CHECK(PmFiltersDesigner<double>::createBandpassFilter(
        taps,
        (NoiseExtractorFrequency - NoiseExtractorStopband) / IntermediateSamplingRate,
        (NoiseExtractorFrequency - NoiseExtractorBandwidth) / IntermediateSamplingRate,
        (NoiseExtractorFrequency + NoiseExtractorBandwidth) / IntermediateSamplingRate,
        (NoiseExtractorFrequency + NoiseExtractorStopband) / IntermediateSamplingRate,
        Ripple,
        Attenuation,
        Gain,
        FirFilterType(1),
        // Used together with SNR estimator filters,
        // so compensated delay is necessary.
        CompensatedDelayAlignment
    ));
    writer.writeTaps("NoiseExtractor", taps);

#ifdef INTEGRATE_BANDWIDTH
    taps2.assign(taps.begin(), taps.end());
    LOG(ERROR) << "NoiseExtractor Integral = " << integrate(taps2) <<
        ", naive calculation = " <<
        (NoiseExtractorBandwidth + NoiseExtractorStopband) / IntermediateSamplingRate;
#endif // INTEGRATE_BANDWIDTH

    return 0;
}
