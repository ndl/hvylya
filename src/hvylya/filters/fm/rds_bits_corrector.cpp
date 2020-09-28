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

namespace {

enum: std::int32_t
{
    // x^10 + x^8 + x^7 + x^5 + x^4 + x^3 + 1
    Generator = 0x5B9,
    // x^9 + x^8 + x^4 + x^3 + x + 1
    Poly = 0x31B,
    BlockBitsCount = 26,
    BlockMask = (1 << BlockBitsCount) - 1,
    InfoBitsCount = 16,
    ParityBitsCount = BlockBitsCount - InfoBitsCount,
    MaxDegreeMask = 1 << ParityBitsCount,
    // Need one extra bit to account for MaxDegreeMask bit.
    ParityMask = (1 << (ParityBitsCount + 1)) - 1,
    SyndromeMask = (1 << ParityBitsCount) - 1,
    // 5 least-significant 1s
    TrapMask = ((1 << 5) - 1)
};

std::int32_t calculateParity(std::int32_t info)
{
    std::int32_t parity = 0;

    // Divide by Generator.
    for (std::int32_t i = InfoBitsCount - 1; i >= 0; --i)
    {
        parity =
            (info & (1 << i)) ?
            ((parity << 1) ^ MaxDegreeMask) & ParityMask :
            (parity << 1) & ParityMask;

        if (parity & MaxDegreeMask)
        {
            parity = (parity ^ Generator) & ParityMask;
        }
    }

    return parity;
}

std::int32_t calculateSyndrome(std::int32_t block)
{
    std::int32_t syndrome = 0;

    // Multiply by Poly and divide by Generator.
    for (std::int32_t i = BlockBitsCount - 1; i >= 0; --i)
    {
        syndrome =
            (block & (1 << i)) ?
            ((syndrome << 1) ^ Poly) & ParityMask :
            (syndrome << 1) & ParityMask;

        if (syndrome & MaxDegreeMask)
        {
            syndrome = (syndrome ^ Generator) & ParityMask;
        }
    }

    return syndrome;
}

} // anonymous namespace

std::int32_t hvylya::filters::encodeRdsBlock(std::int32_t info, std::int32_t offset)
{
    std::int32_t parity = calculateParity(info);
    return (info << ParityBitsCount) | (parity ^ offset);
}

bool hvylya::filters::isRdsBlockValid(std::int32_t block, std::int32_t offset)
{
    return !calculateSyndrome(block ^ offset);
}

hvylya::filters::RdsDecodingStatus hvylya::filters::decodeRdsBlock(std::int32_t& info, std::int32_t block, std::int32_t offset)
{
    block ^= offset;
    std::int32_t syndrome = calculateSyndrome(block);

    if (!syndrome)
    {
        info = block >> ParityBitsCount;
        return RdsDecodingStatus::Valid;
    }

    for (std::int32_t i = 0; i < BlockBitsCount; ++i)
    {
        if (!(syndrome & TrapMask))
        {
            // Avoid undefined behavior by treating these as unsigned.
            std::uint32_t syndrome_padded = static_cast<std::uint32_t>(syndrome) << InfoBitsCount;
            std::uint32_t errors = ((syndrome_padded << (BlockBitsCount - i)) | (syndrome_padded >> i)) & BlockMask;
            std::int32_t corrected_block = block ^ static_cast<std::int32_t>(errors);
            std::int32_t corrected_syndrome = calculateSyndrome(corrected_block);
            if (corrected_syndrome || (errors & SyndromeMask))
            {
                // Either further errors are present or the error
                // is detected in the parity block - give up.
                return RdsDecodingStatus::Uncorrectable;
            }
            else
            {
                // No further errors detected - extract info and return.
                info = corrected_block >> ParityBitsCount;
                return RdsDecodingStatus::Corrected;
            }
        }
        else
        {
            syndrome = (syndrome << 1) & ParityMask;
            if (syndrome & MaxDegreeMask)
            {
                syndrome = (syndrome ^ Generator) & ParityMask;
            }
        }
    }

    return RdsDecodingStatus::Uncorrectable;
}
