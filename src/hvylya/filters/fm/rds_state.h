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

#include <hvylya/core/fastsig.h>
#include <hvylya/filters/fm/rds_decoding_status.h>

namespace hvylya {
namespace filters {

struct RdsBlock;

struct RdsCheckData
{
    std::uint64_t data;
    RdsDecodingStatus status;

    RdsCheckData();

    RdsCheckData(std::uint64_t data, RdsDecodingStatus status);

    // Allow implicit construction.
    RdsCheckData(const RdsBlock& block);
};

enum class ValueTag
{
    Programme,
    Clock,
    Text,
    Current
};

template <typename T, ValueTag Tag>
class RdsValue
{
  public:
    typedef core::Signal<void (T)> ChangingSignal;
    typedef core::Signal<void ()> ExpiredSignal;

    RdsValue():
        has_corrected_(false),
	sent_(false),
	last_update_(std::time_t())
    {
    }

    RdsValue(T value, RdsCheckData check_data):
        RdsValue()
    {
        set(value, check_data);
    }

    void set(T value, RdsCheckData check_data)
    {
        if (check_data.status > RdsDecodingStatus::Uncorrectable)
        {
	    bool is_valid = false;
	    if (check_data.status == RdsDecodingStatus::Valid)
	    {
	        is_valid = true;
	    }
	    else if (has_corrected_)
	    {
	        is_valid = (last_check_data_ == check_data.data);
	    }
	    else
	    {
	        is_valid = (sent_ && sent_value_ == value);
	        has_corrected_ = true;
	    }

	    last_check_data_ = check_data.data;
            last_value_ = value;

	    if (is_valid)
	    {
	        if (!sent_ || !(sent_value_ == value))
	        {
	            changing_(value);
	        }

                sent_ = true;
	        sent_value_ = value;
	        has_corrected_ = false;
                last_update_ = std::time(nullptr);
	    }
        }
    }

    bool valid() const
    {
        return sent_;
    }

    void invalidate()
    {
        if (sent_)
	{
	    expired_();
	    sent_ = false;
            last_update_ = std::time_t();
	}
    }

    const T& value() const
    {
        CHECK(sent_);
	return sent_value_;
    }

    const T& lastValue() const
    {
        CHECK(sent_ || has_corrected_);
        return last_value_;
    }

    std::time_t lastUpdate() const
    {
        return last_update_;
    }

    ChangingSignal& changing()
    {
        return changing_;
    }

    ExpiredSignal& expired()
    {
        return expired_;
    }

  private:
    bool has_corrected_, sent_;
    T sent_value_, last_value_;
    std::uint64_t last_check_data_;
    std::time_t last_update_;
    ChangingSignal changing_;
    ExpiredSignal expired_;
};

typedef RdsValue<std::int32_t, ValueTag::Programme> FreqRdsValue;
typedef RdsValue<std::uint32_t, ValueTag::Programme> TdcRdsValue;

struct ProgrammeItemStartTime
{
    std::uint8_t day_of_month, hour, minute;

    bool operator ==(const ProgrammeItemStartTime& other) const
    {
        return day_of_month == other.day_of_month &&
	    hour == other.hour &&
	    minute == other.minute;
    }
};

struct EonProgrammeInfo
{
    std::vector<RdsValue<char, ValueTag::Text>> programme_service_name;
    std::vector<FreqRdsValue> frequencies;
    RdsValue<bool, ValueTag::Programme> traffic_programme;
    RdsValue<bool, ValueTag::Current> traffic_announcement;
    RdsValue<std::uint16_t, ValueTag::Programme> programme_type;
    RdsValue<ProgrammeItemStartTime, ValueTag::Programme> programme_item_start_time;
    RdsValue<bool, ValueTag::Programme> linkage_actuator, extended_generic, i13l_linkage_set;
    RdsValue<std::uint16_t, ValueTag::Programme> linkage_set_number;
};

const std::uint16_t TmcContinuityCount = 6;
const std::uint16_t TmcMaxExtraGroupsCount = 3;
const std::time_t TmcImmediateTimeout = 15;

struct TmcCheckData
{
    std::vector<std::uint64_t> data;
    RdsDecodingStatus status;

    TmcCheckData() { }

    // Implicit construction.
    TmcCheckData(const RdsCheckData& check_data):
        data(1, check_data.data),
        status(check_data.status)
    {
    }
};

struct TmcMessage
{
    std::uint16_t duration;
    bool diversion;
    bool negative_direction;
    std::uint16_t extent;
    std::uint16_t event;
    std::uint16_t location;
    std::uint16_t total_extra_groups;
    std::vector<std::uint32_t> extra;
};

struct TmcPendingMessage
{
    TmcCheckData check_data;
    TmcMessage message;

    TmcPendingMessage(const TmcCheckData& data, const TmcMessage& msg):
        check_data(data),
        message(msg)
    {
    }
};

struct TmcFirstGroup
{
    TmcMessage message;
    std::time_t timestamp;
    std::uint16_t count;
};

struct TmcSecondGroup
{
    TmcMessage message;
    std::time_t timestamp;
    std::uint16_t count, remaining_groups;
    std::uint32_t data;
};

struct TmcNextGroup
{
    TmcMessage message;
    std::time_t timestamp;
    std::uint16_t count;
    std::uint32_t data;
};

struct TmcChannel
{
    std::vector<FreqRdsValue> frequencies;
    RdsValue<std::uint16_t, ValueTag::Programme> programme_identification, ltn, sid;
    RdsValue<bool, ValueTag::Programme> scope_i13l, scope_national, scope_regional, scope_urban;
};

struct RdsState
{
    typedef RdsValue<std::uint16_t, ValueTag::Programme> OdaAidRdsValue;

    RdsState();

    RdsState(const RdsState& other) = default;

    RdsState(RdsState&& other) = default;

    RdsState& operator =(RdsState&& other) = default;

    void clear(bool everything = true, std::time_t cutoff_time = 0);

    void dump() const;

    // Section 2.2
    RdsValue<std::uint16_t, ValueTag::Programme> programme_identification, programme_type;
    RdsValue<bool, ValueTag::Programme> traffic_programme;

    // Section 3.1.5.1
    RdsValue<bool, ValueTag::Current> traffic_announcement, music_speech, stereo, artificial_head, compressed, dynamic_pty;
    std::vector<FreqRdsValue> alternative_frequencies;
    std::vector<RdsValue<char, ValueTag::Text>> programme_service_name;

    // Section 3.1.5.2
    RdsValue<const char*, ValueTag::Programme> country, language;
    // TODO: figure out what these are.
    // RdsValue<std::uint16_t> tmc_id, ews_channel;
    RdsValue<bool, ValueTag::Programme> linkage_actuator;
    RdsValue<ProgrammeItemStartTime, ValueTag::Programme> programme_item_start_time;

    // Section 3.1.5.3
    std::vector<RdsValue<char, ValueTag::Text>> radio_text;
    RdsValue<bool, ValueTag::Text> text_ab;

    // Section 3.1.5.4
    RdsValue<std::uint16_t, ValueTag::Programme> oda_message;
    // App group type -> ODA AID mapping
    std::unordered_map<std::uint16_t, OdaAidRdsValue> oda_aids;

    // Section 3.1.5.6
    RdsValue<std::time_t, ValueTag::Clock> current_time;

    // Section 3.1.5.8
    std::unordered_map<std::uint16_t, TdcRdsValue> transparent_data_channels;

    // Section 3.1.5.12
    std::unordered_map<std::uint64_t, TmcFirstGroup> tmc_multigroups_first[TmcContinuityCount];
    std::unordered_map<std::uint64_t, TmcSecondGroup> tmc_multigroups_second[TmcContinuityCount];
    std::unordered_map<std::uint64_t, TmcNextGroup> tmc_multigroups_next[TmcContinuityCount][TmcMaxExtraGroupsCount];
    std::unordered_map<std::uint64_t, TmcPendingMessage> tmc_pending_messages;

    std::vector<RdsValue<char, ValueTag::Programme>> tmc_service_provider_name;
    std::vector<TmcChannel> tmc_channels;
    RdsValue<std::uint16_t, ValueTag::Programme> tmc_ltn, tmc_gap, tmc_sid, tmc_activity_time, tmc_window_time, tmc_delay_time;
    RdsValue<bool, ValueTag::Programme> tmc_afi, tmc_mode, tmc_scope_i13l, tmc_scope_national, tmc_scope_regional, tmc_scope_urban;

    // Section 3.1.5.14
    std::vector<RdsValue<char, ValueTag::Text>> programme_type_name;
    RdsValue<bool, ValueTag::Text> ptn_ab;

    // Section 3.1.5.19
    std::unordered_map<std::uint16_t, EonProgrammeInfo> eon_mapping;

    // ODA DAB linkage, see ETSI EN 301 700
    RdsValue<std::uint16_t, ValueTag::Programme> dab_eid, dab_sid, dab_mode, dab_link_linkage_set_number;
    RdsValue<bool, ValueTag::Programme> dab_link_linkage_actuator, dab_link_soft_hard, dab_link_extended_generic, dab_link_i13l_linkage_set;
    RdsValue<std::uint32_t, ValueTag::Programme> dab_freq;
};

} // namespace filters
} // namespace hvylya
