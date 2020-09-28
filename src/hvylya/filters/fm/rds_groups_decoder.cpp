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
#include <hvylya/filters/fm/rds_groups_decoder.h>

using namespace hvylya::filters;

namespace {

enum: int32_t
{
    BlockBitsCount = 26,
    BlockMask = (1 << BlockBitsCount) - 1,
    GroupBitsCount = 4 * BlockBitsCount,
    MinBlocksExtractedToSync = 4,
    MinBlocksValidToSync = 2,
    MinBlocksExtractedWhenSynced = 3,
    MinBlocksValidWhenSynced = 1,
    MaxFailedBlocks = 32
};

const std::int32_t Offsets[4] = { 0xFC, 0x198, 0x168, 0x1B4 };
const std::int32_t OffsetVersionB = 0x350;

} // anonymous namespace

RdsGroupsDecoder::RdsGroupsDecoder()
{
    reset();
    // Use smaller buffering for our input.
    Base::inputState(0).setSuggestedSize(GroupBitsCount);
    // Turn off buffering for our output.
    Base::outputState(0).setSuggestedSize(1);
    // If we don't detect RDS groups, we won't have any output - let
    // pipeline know about it.
    Base::outputState(0).setProvidedSize(0);
}

void RdsGroupsDecoder::reset()
{
    Base::reset();
    accumulated_bits_ = 0;
    recent_failed_blocks_ = 0;
    synced_ = false;
    stats_.clear();
    std::fill(&blocks_[0], &blocks_[3], 0);
}

void RdsGroupsDecoder::process(const typename Base::Inputs& input, typename Base::Outputs& output)
{
    const auto& input_data = std::get<0>(input);
    auto& output_data = std::get<0>(output);

    const std::size_t input_data_size = input_data.size();
    const std::size_t output_data_size = output_data.size();

    std::size_t input_index = 0, output_index = 0;
    for (; input_index < input_data_size && output_index < output_data_size; ++input_index)
    {
        addData(input_data[input_index]);

        {
            std::unique_lock<std::mutex> lock(stats_mutex_);
            stats_.tentative_skipped_bits = accumulated_bits_;
        }

        if (accumulated_bits_ >= GroupBitsCount)
        {
            // Try to decode all 4 blocks and see if this looks like the valid sync position.
            RdsGroup group;
            std::size_t blocks_invalid = 0, blocks_corrected = 0, blocks_valid = 0;

            for (std::uint32_t offset = 0; offset < 4; ++offset)
            {
                switch (extractInfo(group, offset))
                {
                    case RdsDecodingStatus::Uncorrectable:
                        ++blocks_invalid;
                        break;

                    case RdsDecodingStatus::Corrected:
                        ++blocks_corrected;
                        break;

                    case RdsDecodingStatus::Valid:
                        ++blocks_valid;
                        break;
                }
            }

            if (synced_ && !(accumulated_bits_ % GroupBitsCount))
            {
                recent_failed_blocks_ = (blocks_valid > 0) ? 0 : recent_failed_blocks_ + blocks_invalid;
                if (recent_failed_blocks_ > MaxFailedBlocks)
                {
                    synced_ = false;
                }
            }

            if ((blocks_corrected + blocks_valid >= MinBlocksExtractedToSync &&
                 blocks_valid >= MinBlocksValidToSync) ||
                (synced_ &&
                 !(accumulated_bits_ % GroupBitsCount) &&
                 blocks_corrected + blocks_valid >= MinBlocksExtractedWhenSynced &&
                 blocks_valid >= MinBlocksValidWhenSynced))
            {
                std::unique_lock<std::mutex> lock(stats_mutex_);

                synced_ = true;
                stats_.skipped_bits += accumulated_bits_ - GroupBitsCount;
                stats_.tentative_skipped_bits = 0;
                stats_.failed_blocks += blocks_invalid;
                stats_.corrected_blocks += blocks_corrected;
                stats_.valid_blocks += blocks_valid;

                accumulated_bits_ = 0;

                // Output current group:
                output_data[output_index++] = group;
            }
        }
    }

    input_data.advance(input_index);
    output_data.advance(output_index);
}

void RdsGroupsDecoder::addData(std::int8_t data)
{
    for (std::size_t i = 0; i < 3; ++i)
    {
        blocks_[i] = ((blocks_[i] << 1) | ((blocks_[i + 1] & (1 << (BlockBitsCount - 1))) ? 1 : 0)) & BlockMask;
    }
    blocks_[3] = ((blocks_[3] << 1) | data) & BlockMask;
    ++accumulated_bits_;
}

RdsDecodingStatus RdsGroupsDecoder::extractInfo(RdsGroup& group, std::uint32_t offset)
{
    std::int32_t data = 0;
    RdsDecodingStatus status = decodeRdsBlock(data, blocks_[offset], Offsets[offset]);

    // Blocks C can come in two versions as determined by offsets patterns.
    // Try to decode using both offsets and see which one works.
    if (offset == 2)
    {
        std::int32_t data2 = 0, version_b = 0;
        RdsDecodingStatus status2 = decodeRdsBlock(data2, blocks_[offset], OffsetVersionB);
        if (status2 >= status)
        {
            data = data2;
            status = status2;
            version_b = 1;
        }

        // Use the fact that version of the block is also specified in the payload,
        // if that version doesn't match our flag - there was an error in decoding.
        //
        // Note that because we're decoding offsets sequentially and right now we're at
        // offset = 2, it's safe to access index 1.
        if (
            status >= RdsDecodingStatus::Corrected &&
            group[1].status != RdsDecodingStatus::Uncorrectable &&
            ((group[1].data >> 11) & 1) != version_b
        )
        {
            ((group[1].status <= status) ? group[1].status : status) = RdsDecodingStatus::Uncorrectable;
        }
    }

    // Always set these, even if the status is Uncorrectable, so that the output is deterministic.
    group[offset].status = status;
    group[offset].data = static_cast<std::uint16_t>(data);

    return status;
}

RdsDecodingStats RdsGroupsDecoder::stats() const
{
    std::unique_lock<std::mutex> lock(stats_mutex_);
    RdsDecodingStats stats(stats_);
    return stats;
}
