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
#include <hvylya/filters/fm/cma_equalizer.h>
#include <hvylya/filters/fm/rds_decoding_stats.h>
#include <hvylya/filters/fm/rds_bits_decoder.h>
#include <hvylya/filters/fm/rds_demodulator.h>
#include <hvylya/filters/sdr_kernel_source.h>
#include <hvylya/filters/file_source.h>
#include <hvylya/filters/alsa_sink.h>
#include <hvylya/filters/file_sink.h>
#include <hvylya/filters/null_sink.h>
#include <hvylya/filters/connect.h>

#include <hvylya/pipelines/async/pipeline.h>

#include <fm-receiver/stations.h>

// TODO: remove, for debugging only.
#include <iostream>

#include <ncurses.h>

using namespace hvylya::filters;
using namespace hvylya::filters::fm;
using namespace hvylya::pipelines::async;

using namespace fm_receiver;

namespace {

void dumpRdsStats(const RdsDecodingStats& stats)
{
    std::size_t total_skipped_bits = stats.skipped_bits + stats.tentative_skipped_bits;
    std::size_t total_blocks = stats.failed_blocks + stats.corrected_blocks + stats.valid_blocks;

    std::cout << "* blocks = " << total_blocks <<
        ", failed blocks = " << stats.failed_blocks <<
        ", corrected blocks = " << stats.corrected_blocks <<
        ", valid blocks = " << stats.valid_blocks <<
        ", bits = " << total_skipped_bits + 26 * total_blocks <<
        ", skipped bits = " << total_skipped_bits << std::endl;

#ifdef RDS_DUMP_DEMOD_STATS
    std::cout << "rds demod real = " << g_rds_demod_real << ", imag = " << g_rds_demod_imag << std::endl;
#endif // RDS_DUMP_DEMOD_STATS

#ifdef RDS_DUMP_CLOCK_STATS
    std::cout << "clock stats =";
    for (std::size_t i = 0; i < 16 * 4; ++i)
    {
        std::cout << " " << g_clocks_stats[i];
    }
    std::cout << std::endl;
#endif // RDS_DUMP_CLOCK_STATS

#ifdef DUMP_CMA_EQUALIZER_STATS
    std::cout << "Total samples = " << g_samples << ", taps updates = " << g_taps_updates << ", ratio = " << double(g_samples) / double(g_taps_updates) <<
        ", gain udates = " << g_gain_updates << ", ratio = " << double(g_samples) / double(g_gain_updates) << std::endl;
#endif // DUMP_CMA_EQUALIZER_STATS
}

void dumpSnrStats(const FmReceiver<float>& fm_receiver)
{
    std::cout <<
        "Pilot SNR: " << fm_receiver.pilotSnr() <<
        ", Mono SNR: " << fm_receiver.monoSnr() <<
        ", Stereo SNR: " << fm_receiver.stereoSnr() <<
        ", RDS SNR: " << fm_receiver.rdsSnr() <<
        std::endl;
}

void runScanPipeline(const char* device)
{
    SdrKernelSource<float> source(device);
    std::vector<Station> stations;
    findStations(stations, source);

    for (const auto& station: stations)
    {
        std::cout << "Freq = " << float(station.frequency()) / 1000000 << ", SNR = " << station.snr() << std::endl;
    }
}

void runDumpPipeline(std::size_t frequency, std::size_t delay)
{
    Pipeline pipeline;

    SdrKernelSource<float> source("/dev/swradio0");
    source.setSamplingRate(1000000);
    source.setCenterFrequency(frequency);

    FileSink<std::complex<float>, 1> sink(fmt::format("/usr/src/carpc.extra/samples/hvylya/dump_{0}-f32x2@1000000.bin", frequency / 1000000.0).c_str());
    connect(source, sink);

    pipeline.add(source);

    source.startStreaming();

    pipeline.start();
    sleep(unsigned(delay));
    pipeline.stop();
}

void updateFrequency(Pipeline& pipeline, SdrKernelSource<float>& source, std::size_t frequency)
{
    pipeline.pause();
    source.setCenterFrequency(frequency);
    pipeline.reset();
    pipeline.resume();
}

void runLivePipeline(std::size_t frequency)
{
    // Increase extra queue depth to avoid too many threads wake ups.
    Pipeline pipeline(10);

    FmReceiver<float> fm_receiver;

    SdrKernelSource<float> source("/dev/swradio0");
    source.setSamplingRate(fm_receiver.inputSamplingRate());
    source.setCenterFrequency(frequency);

    AlsaSink<float, 2> sink(
        "plughw:1,0",
        fm_receiver.outputAudioSamplingRate(),
        fm_receiver.outputAudioSamplingRate() / 5 // =0.2s delay
    );

    connect(source, fm_receiver);

    connect(makeChannel<0>(fm_receiver), makeChannel<0>(sink));
    connect(makeChannel<1>(fm_receiver), makeChannel<1>(sink));

    pipeline.add(source);

    source.startStreaming();

    pipeline.start();

    initscr();
    keypad(stdscr, TRUE);
    noecho();

    bool use_stereo = true;
    bool running = true;
    while (running)
    {
        switch (getch())
        {
            case KEY_LEFT:
                frequency -= 100000;
                std::cout << "New frequency: " << frequency / 1000000.0 << std::endl;
                updateFrequency(pipeline, source, frequency);
                break;

            case KEY_NPAGE:
                frequency -= 1000000;
                std::cout << "New frequency: " << frequency / 1000000.0 << std::endl;
                updateFrequency(pipeline, source, frequency);
                break;

            case KEY_RIGHT:
                frequency += 100000;
                std::cout << "New frequency: " << frequency / 1000000.0 << std::endl;
                updateFrequency(pipeline, source, frequency);
                break;

            case KEY_PPAGE:
                frequency += 1000000;
                std::cout << "New frequency: " << frequency / 1000000.0 << std::endl;
                updateFrequency(pipeline, source, frequency);
                break;

            case 's':
                use_stereo = !use_stereo;
                fm_receiver.setStereoWeight(use_stereo ? 1.0f : 0.0f);
                break;

            case 'n':
                dumpSnrStats(fm_receiver);
                break;

            case 'r':
                dumpRdsStats(fm_receiver.rdsDecodingStats());
                fm_receiver.rdsState().dump();
                break;

            case 'q':
            case 'x':
                running = false;
                std::cout << "Exiting ..." << std::endl;
                break;

            default:
                break;
        }
    }

    endwin();

    pipeline.stop();
}

void runTestPipeline()
{
    const char* samples[] =
    {
        "/usr/src/carpc.extra/samples/hvylya/sample_signal.bin",
        "/usr/src/carpc.extra/samples/hvylya/iq-fm-96.0_resampled.bin",
        ////Captured off-frequency to test FirTranslatingFilter:
        //"/usr/src/carpc.extra/samples/hvylya/sample_signal3_94_5.bin",
        "/usr/src/carpc.extra/samples/hvylya/sample_signal4_99_6_rds.bin",
        "/usr/src/carpc.extra/samples/hvylya/sample_signal5_94_6_rds.bin",
        "/usr/src/carpc.extra/samples/hvylya/sample_signal6_90_0.bin",
        "/usr/src/carpc.extra/samples/hvylya/sample_signal7_93_6.bin",
        "/usr/src/carpc.extra/samples/hvylya/sample_signal8_94_6.bin",
        "/usr/src/carpc.extra/samples/hvylya/sample_signal9_106_7.bin",
        "/usr/src/carpc.extra/samples/hvylya/sample_signal12_88.4-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/sample_signal13_91.9-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/sample_signal14_93.6-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya.new/old/dump_90.5-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/piano2-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/piano2_with_noise-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/piano2_multipath-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/piano2_multipath2-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya.new/dump_100.7-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya.new/dump_90.5-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya.new/dump_93.5-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d1-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d2-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d3-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d4-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d5-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d6-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d7-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d8-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d9-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d10-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d11-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d12-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d13-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d14-f32x2@1000000.bin",
        "/usr/src/carpc.extra/samples/hvylya/multipath_d15-f32x2@1000000.bin",
        nullptr
    };

    std::vector<std::tuple<std::size_t, double>> stats;

    for (std::size_t i = 0; samples[i]; ++i)
    {
        std::cout << samples[i] << ":" << std::endl;

        Pipeline pipeline;

#ifdef RDS_DUMP_CLOCK_STATS
        for (std::size_t j = 0; j < 16 * 4; ++j)
        {
            g_clocks_stats[j] = 0;
        }
#endif // RDS_DUMP_CLOCK_STATS

#ifdef DUMP_CMA_EQUALIZER_STATS
        g_samples = g_taps_updates = g_gain_updates = 0;
#endif // DUMP_CMA_EQUALIZER_STATS

        FileSource<std::complex<float>> source(samples[i], false);
        FmReceiver<float> fm_receiver;

        //FileSink<float, 2> sink("pipeline_output_stereo.bin");
        NullSink<float, 2> sink;
 
        connect(source, fm_receiver);

        connect(makeChannel<0>(fm_receiver), makeChannel<0>(sink));
        connect(makeChannel<1>(fm_receiver), makeChannel<1>(sink));

        pipeline.add(source);
        pipeline.run();

        dumpSnrStats(fm_receiver);
        dumpRdsStats(fm_receiver.rdsDecodingStats());

        stats.emplace_back(fm_receiver.rdsDecodingStats().valid_blocks, 0);

        std::cout << std::endl;
    }

    std::cout << "Stats summary:" << std::endl;

    for (std::size_t i = 0; i < stats.size(); ++i)
    {
        std::cout << std::get<0>(stats[i]) << " (" << std::get<1>(stats[i]) << "), ";
    }
    std::cout << std::endl;
}

void runLoadPipeline(const char* file_path)
{
    // Increase extra queue depth to avoid too many threads wake ups.
    Pipeline pipeline(10);

    FmReceiver<float> fm_receiver;

    FileSource<std::complex<float>> source(file_path, false);

    AlsaSink<float, 2> sink(
        "plughw:1,0",
        fm_receiver.outputAudioSamplingRate(),
        fm_receiver.outputAudioSamplingRate() / 5 // =0.2s delay
    );

    connect(source, fm_receiver);

    connect(makeChannel<0>(fm_receiver), makeChannel<0>(sink));
    connect(makeChannel<1>(fm_receiver), makeChannel<1>(sink));

    pipeline.add(source);

    pipeline.start();

    initscr();
    keypad(stdscr, TRUE);
    noecho();

    bool use_stereo = true;
    bool running = true;
    while (running)
    {
        switch (getch())
        {
            case 's':
                use_stereo = !use_stereo;
                fm_receiver.setStereoWeight(use_stereo ? 1.0f : 0.0f);
                break;

            case 'n':
                dumpSnrStats(fm_receiver);
                break;

            case 'r':
                dumpRdsStats(fm_receiver.rdsDecodingStats());
                fm_receiver.rdsState().dump();
                break;

            case 'q':
            case 'x':
                running = false;
                std::cout << "Exiting ..." << std::endl;
                break;

            default:
                break;
        }
    }

    endwin();

    pipeline.stop();
}

} // anonymous namespace

int main(int argc, char* argv[])
{
    google::InitGoogleLogging(argv[0]);

    if (argc == 3 && !strcmp(argv[1], "live"))
    {
        runLivePipeline(std::size_t(std::atoi(argv[2])));
    }
    else if ((argc == 2 || argc == 3) && !strcmp(argv[1], "scan"))
    {
        runScanPipeline(argc == 2 ? "/dev/swradio0" : argv[2]);
    }
    else if (argc == 4 && !strcmp(argv[1], "dump"))
    {
        runDumpPipeline(std::size_t(std::atoi(argv[2])), std::size_t(std::atoi(argv[3])));
    }
    else if (argc == 2 && !strcmp(argv[1], "test"))
    {
        runTestPipeline();
    }
    else if (argc == 3 && !strcmp(argv[1], "load"))
    {
        runLoadPipeline(argv[2]);
    }
    else
    {
        std::cerr << "Unknown program arguments" << std::endl;
    }

    return 0;
}
