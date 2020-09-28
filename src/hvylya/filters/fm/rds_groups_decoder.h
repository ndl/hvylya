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

#pragma once

#include <hvylya/filters/filter_generic.h>
#include <hvylya/filters/fm/rds_decoding_status.h>
#include <hvylya/filters/fm/rds_decoding_stats.h>
#include <hvylya/filters/fm/rds_group.h>

namespace hvylya {
namespace filters {

class RdsGroupsDecoder:
    public FilterGeneric<
        core::TypeList<std::int8_t>,
        core::TypeList<RdsGroup>
    >
{
  public:
    typedef typename FilterBaseType<RdsGroupsDecoder>::Type Base;

    RdsGroupsDecoder();

    virtual void reset() override;

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& output) override;

    RdsDecodingStats stats() const;

  private:
    mutable std::mutex stats_mutex_;
    std::int32_t blocks_[4];
    RdsDecodingStats stats_;
    std::size_t accumulated_bits_, recent_failed_blocks_;
    bool synced_;

    void addData(std::int8_t data);

    RdsDecodingStatus extractInfo(RdsGroup& group, std::uint32_t offset);
};

} // namespace filters
} // namespace hvylya
