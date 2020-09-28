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

#include <hvylya/filters/fm/rds_bits_corrector.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::filters;

namespace {

const std::int32_t BlockBitsCount = 26;
const std::int32_t BlockMask = ((1 << BlockBitsCount) - 1);
const std::int32_t InfoMask = (1 << 16) - 1;
const std::int32_t ParityMask = (1 << 10) - 1;
const std::int32_t MaxDetectableBurst = 10;

const std::int32_t MaxErrorsSites = 3;
const std::int32_t MaxErrorsLen = 26;

// We need enough iterations to gather meaningful stats,
// so need to run this many iterations even with decreased
// TEST_LOAD_FACTOR.
const std::size_t Iterations = 1000000;

// Use fixed implementation of random generator to make sure
// that results are repeatable.
static std::uint32_t seed_det = 0;
std::int32_t randDet()
{
    seed_det = (seed_det * 1103515245u + 12345u) & 0x7FFFFFFFu;
    return static_cast<std::int32_t>(seed_det);
}

std::int32_t createErrorMask()
{
    std::int32_t num_errors_blocks = randDet() % (MaxErrorsSites + 1);
    std::int32_t error_mask = 0;

    for (std::int32_t i = 0; i <= num_errors_blocks; ++i)
    {
        std::int32_t failed_bits_count = randDet() % (MaxErrorsLen + 1);
        std::int32_t failed_bits_pos = failed_bits_count < BlockBitsCount ? randDet() % (BlockBitsCount - failed_bits_count) : 0;
        std::int32_t failed_bits = randDet() % (1 << failed_bits_count);
        error_mask |= failed_bits << failed_bits_pos;
    }

    return error_mask;
}

std::int32_t burstLength(std::int32_t error_mask)
{
    if (!error_mask)
    {
        return 0;
    }

    std::int32_t burst_length = BlockBitsCount;

    for (std::int32_t i = 0; i < BlockBitsCount; ++i)
    {
        burst_length =
            std::min<std::int32_t>(
                burst_length,
                32 - __builtin_clz(unsigned(error_mask)) - __builtin_ctz(unsigned(error_mask))
            );
        error_mask = ((error_mask << 1) | (error_mask >> (BlockBitsCount - 1))) & BlockMask;
    }

    return burst_length;
}

bool isCorrectable(std::int32_t error_mask)
{
    return burstLength(error_mask) <= 5 && !(error_mask & ParityMask);
}

} // anonymous namespace

TEST(RdsBitsCorrector, RegressionTest)
{
    std::size_t bursts_detected[MaxErrorsLen + 1] = { 0 }, bursts_count[MaxErrorsLen + 1] = { 0 };
    std::size_t bursts_corrected[MaxErrorsLen + 1] = { 0 }, bursts_uncorrected[MaxErrorsLen + 1] = { 0 };

    for (std::size_t i = 0; i < Iterations; ++i)
    {
        std::int32_t info = randDet() & InfoMask;
        std::int32_t offset = randDet() & ParityMask;
        std::int32_t encoded_block = encodeRdsBlock(info, offset);

        std::int32_t error_mask = createErrorMask();
        std::int32_t received_block = encoded_block ^ error_mask;

        std::int32_t corrected_info = 0;
        RdsDecodingStatus status = decodeRdsBlock(corrected_info, received_block, offset);

        std::int32_t burst_len = burstLength(error_mask);
        if (status != RdsDecodingStatus::Valid)
        {
            ++bursts_detected[burst_len];
        }
        ++bursts_count[burst_len];

        if (!error_mask)
        {
            EXPECT_EQ(status, RdsDecodingStatus::Valid);
        }
        else if (isCorrectable(error_mask))
        {
            EXPECT_EQ(status, RdsDecodingStatus::Corrected);
            EXPECT_EQ(info, corrected_info);
        }
        else
        {
            // The code should detect all double bit errors.
            std::int32_t num_error_bits = __builtin_popcount(unsigned(error_mask));
            EXPECT_TRUE(num_error_bits > 2 || (error_mask & ParityMask) || status != RdsDecodingStatus::Valid);

            // The code should detect all burst errors of <= 10 bits.
            EXPECT_TRUE(burst_len > MaxDetectableBurst || (error_mask & ParityMask) || status != RdsDecodingStatus::Valid);

            if (status == RdsDecodingStatus::Corrected)
            {
                ++bursts_corrected[burst_len];
            }
            else if (status == RdsDecodingStatus::Uncorrectable)
            {
                ++bursts_uncorrected[burst_len];
            }
        }
    }

    for (std::size_t i = 1; i < BlockBitsCount; ++i)
    {
        ASSERT_GT(bursts_count[i], 1000);

        float detection_rate = float(bursts_detected[i]) / bursts_count[i];
        EXPECT_GT(detection_rate, 0.995);

        if (bursts_corrected[i] || bursts_uncorrected[i])
        {
            float matched_rate = float(bursts_uncorrected[i]) / (bursts_corrected[i] + bursts_uncorrected[i]);
            EXPECT_GT(matched_rate, 0.78);
        }
    }
}
