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

#include <hvylya/filters/file_source.h>
#include <hvylya/filters/fm/rds_bits_corrector.h>
#include <hvylya/filters/fm/rds_groups_decoder.h>
#include <hvylya/filters/fm/rds_messages_decoder.h>
#include <hvylya/filters/connect.h>

#include <hvylya/pipelines/async/pipeline.h>

#include <hvylya/core/tests/common.h>

#include <glob.h>

using namespace hvylya::core;
using namespace hvylya::filters;
using namespace hvylya::pipelines::async;

namespace {

enum class RdsFileFormat
{
    Raw,
    Smp
};

struct RdsFileSpec
{
    std::string file_path_;
    RdsFileFormat format_;

    RdsFileSpec(const char* file_path, RdsFileFormat format):
        file_path_(file_path),
        format_(format)
    {
    }
};

std::ostream& operator << (std::ostream& os, const RdsFileSpec& spec)
{
    os << spec.file_path_;
    return os;
}

class BitsToBytesConverter:
    public FilterGeneric<
        TypeList<std::int8_t>,
        TypeList<std::int8_t>
    >
{
  public:
    typedef typename FilterBaseType<BitsToBytesConverter>::Type Base;

    BitsToBytesConverter()
    {
        // Each input byte is translated into 8 output bytes.
        Base::outputState(0).setRequiredSize(8);
    }

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override
    {
        auto& input_data = std::get<0>(input);
        auto& output_data = std::get<0>(output);
        std::size_t input_size = std::min(input_data.size(), output_data.size() / 8);
        std::size_t output_size = std::min(8 * input_size, output_data.size());

        for (std::size_t output_index = 0; output_index < output_size; ++output_index)
        {
            std::size_t input_index = output_index / 8, input_shift = output_index % 8;
            output_data[output_index] = (input_data[input_index] >> (7 - input_shift)) & 0x01;
        }

        input_data.advance(input_size);
        output_data.advance(output_size);
    }
};

class RdsSmpFileSource:
    public FilterGeneric<
        TypeList<>,
        TypeList<std::int8_t>
    >
{
  public:
    typedef typename FilterBaseType<RdsSmpFileSource>::Type Base;

    RdsSmpFileSource(const char* file_path):
        file_(file_path, std::ios::in | std::ios::binary),
        file_path_(file_path)
    {
        if (!file_)
        {
            THROW(IoError()) << fmt::format("Failed to open the file {0} for reading", file_path_);
        }
        std::uint8_t header[2] = { 0 };
        file_.read(reinterpret_cast<char*>(&header), 2);
        if (header[0] != 0xFF || header[1] != 0x06)
        {
            THROW(IoError()) << fmt::format("Unexpected header in SMP file {0}", file_path_);
        }

        Base::outputState(0).setRequiredSize(26);
    }

    virtual void process(const typename Base::Inputs& /* input */, typename Base::Outputs& output) override
    {
        auto& output_data = std::get<0>(output);
        std::size_t output_size = roundDown<std::size_t>(
            std::min(
                Base::outputState(0).suggestedSize(),
                output_data.size()
            ),
            26
        );

        std::size_t output_index = 0;
        for (; output_index < output_size; output_index += 26)
        {
            std::uint8_t values[4];
            std::size_t read_data =
                std::size_t(
                    file_.read(
                        reinterpret_cast<char*>(&values),
                        std::streamsize(sizeof(values))
                    ).gcount()
                );

            if (!file_ && !file_.eof())
            {
                THROW(IoError()) << fmt::format("Error while reading the file {0}", file_path_);
            }

            if (read_data < sizeof(values))
            {
                Base::outputState(0).setEof(true);
                break;
            }
            else
            {
                for (std::size_t i = 0; i < 26; ++i)
                {
                    output_data[output_index + i] = (values[i / 8] >> (7 - (i % 8))) & 0x01;
                }
            }
        }

        output_data.advance(output_index);
    }

  private:
    std::ifstream file_;
    std::string file_path_;
};

class RdsMessagesDecoderTest: public ::testing::TestWithParam<RdsFileSpec>
{
  public:
    void decode(const RdsFileSpec& spec)
    {
        Pipeline pipeline;

        RdsGroupsDecoder groups_decoder;
        RdsMessagesDecoder messages_decoder;

        switch (spec.format_)
        {
            case RdsFileFormat::Raw:
            {
                FileSource<std::int8_t> file_source(spec.file_path_.c_str(), false);
                BitsToBytesConverter bits_converter;
                connect(file_source, bits_converter, groups_decoder, messages_decoder);
                pipeline.add(file_source);
                pipeline.run();
                break;
            }

            case RdsFileFormat::Smp:
            {
                RdsSmpFileSource file_source(spec.file_path_.c_str());
                connect(file_source, groups_decoder, messages_decoder);
                pipeline.add(file_source);
                pipeline.run();
            }
        }

        RdsDecodingStats stats = groups_decoder.stats();
        //std::size_t total_skipped_bits = stats.skipped_bits + stats.tentative_skipped_bits;
        std::size_t total_blocks = stats.failed_blocks + stats.corrected_blocks + stats.valid_blocks;

        messages_decoder.state().dump();

        EXPECT_GT(total_blocks, 0);
        EXPECT_GT(stats.valid_blocks, 0);
        //EXPECT_EQ(0, stats.failed_blocks);
        //EXPECT_EQ(1, stats.corrected_blocks);
        //EXPECT_EQ(283, stats.valid_blocks);
        //EXPECT_EQ(7450, total_skipped_bits + 26 * total_blocks);
        //EXPECT_EQ(66, total_skipped_bits);
    }
};

std::vector<RdsFileSpec> findRdsFiles()
{
    std::vector<RdsFileSpec> specs;
    glob_t glob_result;
    glob("/usr/src/carpc.extra/samples/hvylya/rds/*/*", 0, nullptr, &glob_result);
    for (std::size_t i = 0; i < glob_result.gl_pathc; ++i)
    {
        const char* path = glob_result.gl_pathv[i];
        const char* ext = path + std::max<std::size_t>(strlen(path), 3) - 3;
        specs.emplace_back(path, !strcasecmp(ext, "smp") ? RdsFileFormat::Smp : RdsFileFormat::Raw);
    }
    globfree(&glob_result);

    return specs;
}

} // anonymous namespace

TEST_P(RdsMessagesDecoderTest, DecodingBasic)
{
    decode(GetParam());
}

DISABLE_WARNING_PUSH("-Wmissing-prototypes")

INSTANTIATE_TEST_SUITE_P(
    RdsSamples,
    RdsMessagesDecoderTest,
    ::testing::ValuesIn(findRdsFiles())
);

DISABLE_WARNING_POP()
