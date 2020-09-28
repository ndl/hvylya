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
#include <hvylya/filters/fm/rds_decoding_stats.h>
#include <hvylya/filters/file_source.h>
#include <hvylya/filters/null_sink.h>
#include <hvylya/filters/connect.h>

#include <hvylya/pipelines/async/pipeline.h>

// Include environment cleaner.
#include <hvylya/filters/tests/fft_test_utils.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::filters;
using namespace hvylya::filters::fm;
using namespace hvylya::pipelines::async;

TEST(FmReceiver, Decoding)
{
    const char* sample = "/usr/src/carpc.extra/samples/hvylya/sample_signal10_generated_piano2-f32x2@1000000.bin";

    Pipeline pipeline;

    FileSource<std::complex<float>> file_source(sample, false);
    FmReceiver<float> fm_receiver;
    NullSink<float, 2> sink;
 
    connect(file_source, fm_receiver);

    connect(makeChannel<0>(fm_receiver), makeChannel<0>(sink));
    connect(makeChannel<1>(fm_receiver), makeChannel<1>(sink));

    pipeline.add(file_source);
    pipeline.run();

    EXPECT_EQ(302485, sink.samples());

    RdsDecodingStats stats = fm_receiver.rdsDecodingStats();
    std::size_t total_skipped_bits = stats.skipped_bits + stats.tentative_skipped_bits;
    std::size_t total_blocks = stats.failed_blocks + stats.corrected_blocks + stats.valid_blocks;

    EXPECT_EQ(284, total_blocks);
    EXPECT_EQ(0, stats.failed_blocks);
    EXPECT_EQ(1, stats.corrected_blocks);
    EXPECT_EQ(283, stats.valid_blocks);
    EXPECT_EQ(7452, total_skipped_bits + 26 * total_blocks);
    EXPECT_EQ(68, total_skipped_bits);
}
