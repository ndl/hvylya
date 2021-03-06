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

#include <hvylya/filters/fm/rds_decoding_stats.h>

using namespace hvylya::filters;

RdsDecodingStats::RdsDecodingStats()
{
    clear();
}

void RdsDecodingStats::clear()
{
    failed_blocks = 0;
    corrected_blocks = 0;
    valid_blocks = 0;
    skipped_bits = 0;
    tentative_skipped_bits = 0;
}
