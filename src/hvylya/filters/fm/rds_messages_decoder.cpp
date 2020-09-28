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

#include <hvylya/filters/fm/rds_messages_decoder.h>
#include <iostream>

// #define PRINT_RDS_STATE

using namespace hvylya::filters;

namespace {

const char* CountriesEurope[5 * 15] =
{
    "DE", "DZ", "AD", "IL", "IT", "BE", "RU", "PS", "AL", "AT", "HU", "MT", "DE", nullptr, "EG",
    "GR", "CY", "SM", "CH", "JO", "FI", "LU", "BG", "DK", "GI", "IQ", "GB", "LY", "RO", "FR",
    "MA", "CZ", "PL", "VA", "SK", "SY", "TN", nullptr, "LI", "IS", "MC", "LT", "YU", "ES", "NO",
    nullptr, "IE", "TR", "MK", nullptr, nullptr, nullptr, "NL", "LV", "LB", nullptr, "HR", nullptr, "SE", "BY",
    "MD", "EE", nullptr, nullptr, nullptr, "UA", nullptr, "PT", "SI", nullptr, nullptr, nullptr, nullptr, nullptr, "BA"
};

const char* LanguagesEurope[0x2B + 1] =
{
    "Unknown",
    "Albanian",
    "Breton",
    "Catalan",
    "Croatian",
    "Welsh",
    "Czech",
    "Danish",
    "German",
    "English",
    "Spanish",
    "Esperanto",
    "Estonian",
    "Basque",
    "Faroese",
    "French",
    "Frisian",
    "Irish",
    "Gaelic",
    "Galician",
    "Icelandic",
    "Italian",
    "Lappish",
    "Latin",
    "Latvian",
    "Luxembourgian",
    "Lithuanian",
    "Hungarian",
    "Maltese",
    "Dutch",
    "Norwegian",
    "Occitan",
    "Polish",
    "Portuguese",
    "Romanian",
    "Romansh",
    "Serbian",
    "Slovak",
    "Slovene",
    "Finnish",
    "Swedish",
    "Turkish",
    "Flemish",
    "Walloon"
};

template <typename Container>
void invalidateContainer(Container& cont)
{
    std::for_each(cont.begin(), cont.end(), [](auto& value) { value.invalidate(); });
}

RdsDecodingStatus typedStatus(const RdsGroup& group, std::size_t index)
{
    // 0th block is type-independent and 1st block doesn't need any correction.
    CHECK(index == 2 || index == 3);
    return std::min(group[1].status, group[index].status);
}

RdsCheckData typedCheckData(const RdsGroup& group, std::size_t index)
{
    RdsCheckData data;
    data.data = std::uint32_t(group[1].data << 16) | group[index].data;
    data.status = typedStatus(group, index);
    return data;
}

RdsCheckData typedCheckDataBoth(const RdsGroup& group)
{
    RdsCheckData check_data = typedCheckData(group, 2);
    check_data.data = (check_data.data << 16) | group[3].data;
    check_data.status = std::min(check_data.status, group[3].status);
    return check_data;
}

RdsCheckData tmcMultiGroupCheckData(const RdsGroup& group)
{
    RdsCheckData check_data = typedCheckDataBoth(group);
    // Mask continuity index - for immediate repetition we check it
    // explicitly and for periodic repetition we don't care.
    check_data.data &= ~(std::uint64_t((1 << 3) - 1) << 32);
    return check_data;
}

void decodeGroupFieldsCommon(RdsState& state, const RdsGroup& group, std::size_t index)
{
    state.programme_identification.set(group[index].data, group[index]);
    state.traffic_programme.set(group[index + 1].data & (1 << 10), group[index + 1]);
    state.programme_type.set((group[index + 1].data >> 5) & ((1 << 5) - 1), group[index + 1]);
}

std::uint16_t decodeGroupCommon(RdsState& state, const RdsGroup& group)
{
    decodeGroupFieldsCommon(state, group, 0);
    return group[1].data >> 11;
}

std::uint16_t decodeTuningInfoCommon(RdsState& state, const RdsGroup& group, std::size_t index)
{
    state.traffic_announcement.set(group[index].data & (1 << 4), group[index]);
    state.music_speech.set(group[index].data & (1 << 3), group[index]);

    bool di = group[index].data & (1 << 3);
    std::uint16_t seg = group[index].data & 0x03;

    switch (seg)
    {
        case 0x00:
            state.stereo.set(di, group[index]);
            break;

        case 0x01:
            state.artificial_head.set(di, group[index]);
            break;

        case 0x02:
            state.compressed.set(di, group[index]);
            break;

        case 0x03:
            state.dynamic_pty.set(di, group[index]);
            break;
    }

    return seg;
}

std::int32_t extractFrequency(std::uint8_t freq_code, bool is_lf_mf_freq = false)
{
    if (is_lf_mf_freq)
    {
        if (freq_code >= 1 && freq_code < 16)
        {
            return 153000 + 9000 * (freq_code - 1);
        }
        else if (freq_code >= 16 && freq_code < 136)
        {
            return 531000 + 9000 * (freq_code - 16);
        }
    }
    else
    {
        if (freq_code > 0 && freq_code < 205)
        {
            return 87600000 + 100000 * (freq_code - 1);
        }
    }

    return 0;
}

void updateFrequency(std::vector<FreqRdsValue>& frequencies, std::int32_t freq, const RdsCheckData& check_data)
{
    auto it = std::find_if(
        frequencies.begin(),
        frequencies.end(),
        [freq](const auto& cur_freq){ return cur_freq.lastValue() == freq; }
    );
    if (it != frequencies.end())
    {
        it->set(freq, check_data);
    }
    else
    {
        frequencies.emplace_back(freq, check_data);
    }
}

void addFrequencies(std::vector<FreqRdsValue>& frequencies, const RdsGroup& group, std::size_t index, bool check_data_both = false)
{
    // Perform an explicit check to avoid inserting wrong values into the container.
    if (group[index].status >= RdsDecodingStatus::Corrected)
    {
        RdsCheckData check_data = check_data_both ? typedCheckDataBoth(group) : typedCheckData(group, index);
        std::uint16_t freqs_codes = group[index].data;
        if ((freqs_codes >> 8) == 250)
        {
            // LF / MF case, first value is the marker and second value is the frequency.
            std::int32_t freq = extractFrequency(freqs_codes & 0xFF, true);
            if (freq)
            {
                updateFrequency(frequencies, freq, check_data);
            }
        }
        else
        {
            // General case, either both values encode FM frequencies or the
            // first value indicates the number of frequencies that follow - but
            // it will be correctly skipped by extractFrequency.
            for (std::size_t i = 0; i < 2; ++i, freqs_codes >>= 8)
            {
                std::int32_t freq = extractFrequency(freqs_codes & 0xFF);
                if (freq)
                {
                    updateFrequency(frequencies, freq, check_data);
                }
            }
        }
    }
}

void decodeGroupType0(RdsState& state, bool ver_b, const RdsGroup& group)
{
    std::uint16_t seg = decodeTuningInfoCommon(state, group, 1);

    if (!ver_b)
    {
        addFrequencies(state.alternative_frequencies, group, 2);
    }
    else
    {
        state.programme_identification.set(group[2].data, group[2]);
    }

    CHECK_LT(2 * seg + 1, state.programme_service_name.size());
    state.programme_service_name[2 * seg].set(char((group[3].data >> 8) & 0xFF), typedCheckData(group, 3));
    state.programme_service_name[2 * seg + 1].set(char(group[3].data & 0xFF), typedCheckData(group, 3));
}

void decodeProgrammeItemStartTime(
    RdsValue<ProgrammeItemStartTime, ValueTag::Programme>& start_time,
    const RdsGroup& group,
    std::size_t index,
    bool check_data_both = false
)
{
    RdsCheckData check_data = check_data_both ? typedCheckDataBoth(group) : typedCheckData(group, index);
    std::uint16_t value = group[index].data;
    ProgrammeItemStartTime pin_time;
    pin_time.day_of_month = (value >> 11) & ((1 << 5) - 1);
    pin_time.hour = (value >> 6) & ((1 << 5) - 1);
    pin_time.minute = value & ((1 << 6) - 1);
    start_time.set(pin_time, check_data);
}

void decodeTmcSystemInfoCommon(RdsState& state, const RdsGroup& group, RdsCheckData& check_data);

void decodeGroupType1(RdsState& state, bool ver_b, const RdsGroup& group)
{
    if (!ver_b)
    {
        if (group[2].status >= RdsDecodingStatus::Corrected)
        {
            std::uint16_t code_selector = (group[2].data >> 12) & 0x07;
            state.linkage_actuator.set(group[2].data & (1 << 15), typedCheckData(group, 2));

            switch (code_selector)
            {
                case 0x00:
                {
                    if (state.programme_identification.valid())
                    {
                        std::uint16_t cc = state.programme_identification.value() >> 12;
                        std::uint16_t ecc = group[2].data & 0xFF;

                        if (ecc >= 0xE0 && ecc <= 0xE4 && cc > 0)
                        {
                            CHECK_LT(15 * (ecc - 0xE0) + cc - 1, sizeof(CountriesEurope) / sizeof(CountriesEurope[0]));
                            state.country.set(CountriesEurope[15 * (ecc - 0xE0) + cc - 1], typedCheckData(group, 2));
                        }
                    }
                    break;
                }

                case 0x01: // Not used for TMC ODA, present here only to decode historical files.
                {
                    RdsCheckData check_data = typedCheckData(group, 2);
                    decodeTmcSystemInfoCommon(state, group, check_data);
                    break;
                }

                case 0x02: // Paging identification - we don't support paging decoding, so skip it.
                    break;

                case 0x03:
                {
                    std::uint16_t lang_code = group[2].data & ((1 << 11) - 1);
                    if (lang_code < sizeof(LanguagesEurope) / sizeof(LanguagesEurope[0]))
                    {
                        state.language.set(LanguagesEurope[lang_code], typedCheckData(group, 2));
                    }
                    break;
                }

                case 0x06: // "For use by broadcasters ... shall entirely ignore this information."
                case 0x07: // Emergency Warning System - unfortunately, the format is not specified in the standard.
                    break;

                default:
                    if (group[1].status == RdsDecodingStatus::Valid && group[2].status == RdsDecodingStatus::Valid)
                    {
                        LOG(ERROR) << "Unexpected code selector " << code_selector << " for 1A group decoding";
                    }
            }
        }
    }
    else
    {
        state.programme_identification.set(group[2].data, group[2]);
    }

    decodeProgrammeItemStartTime(state.programme_item_start_time, group, 3);
}

void decodeGroupType2(RdsState& state, bool ver_b, const RdsGroup& group)
{
    state.text_ab.set(group[1].data & (1 << 4), group[1]);

    std::uint16_t seg = group[1].data & ((1 << 4) - 1);
    std::uint16_t index = 4 * seg;

    if (!ver_b)
    {
        CHECK_LT(index + 1, state.radio_text.size());
        state.radio_text[index].set(char((group[2].data >> 8) & 0xFF), typedCheckData(group, 2));
        state.radio_text[index + 1].set(char(group[2].data & 0xFF), typedCheckData(group, 2));
        index += 2;
    }
    else
    {
        index = 2 * seg;
        state.programme_identification.set(group[2].data, group[2]);
    }

    CHECK_LT(index + 1, state.radio_text.size());
    state.radio_text[index].set(char((group[3].data >> 8) & 0xFF), typedCheckData(group, 3));
    state.radio_text[index + 1].set(char(group[3].data & 0xFF), typedCheckData(group, 3));
}

void decodeOpenDataApplicationInitMessage(RdsState& state, const RdsGroup& group);

void decodeGroupType3A(RdsState& state, const RdsGroup& group)
{
    decodeOpenDataApplicationInitMessage(state, group);

    // Perform an explicit check to avoid inserting wrong values into the container.
    if (group[3].status >= RdsDecodingStatus::Corrected)
    {
        std::uint16_t app_group_type = group[1].data & ((1 << 5) - 1);
        std::uint16_t aid = group[3].data;
        auto it = state.oda_aids.find(app_group_type);
        if (it != state.oda_aids.end())
        {
            it->second.set(aid, typedCheckData(group, 3));
        }
        else
        {
            state.oda_aids[app_group_type] = RdsState::OdaAidRdsValue(aid, typedCheckData(group, 3));
        }
    }
}

void decodeGroupType4A(RdsState& state, const RdsGroup& group)
{
    if (group[2].status >= RdsDecodingStatus::Corrected &&
        group[3].status >= RdsDecodingStatus::Corrected)
    {
        std::uint32_t mjd = (std::uint32_t(group[1].data & 0x03) << 15) | (group[2].data >> 1);
        std::uint16_t hour = std::uint16_t(((group[2].data & 1) << 4) | (group[3].data >> 12));
        std::uint16_t minute = (group[3].data >> 6) & ((1 << 6) - 1);
        //std::int8_t time_offset = group[3].data & ((1 << 5) - 1);
        //std::int8_t time_offset_dir = group[3].data & (1 << 5) ? -1 : 1;

        int tmp_year = int((mjd - 15078.2) / 365.25);
        int tmp_month = int((mjd - 14956.1 - int(tmp_year * 365.25)) / 30.6001);
        int day = int(mjd) - 14956 - int(tmp_year * 365.25) - int(tmp_month * 30.6001);
        int k = (tmp_month == 14 || tmp_month == 15) ? 1 : 0;
        int year = tmp_year + k;
        int month = tmp_month - 1 - 12 * k;

        struct tm cur_time;
        cur_time.tm_year = year;
        cur_time.tm_mon = month - 1;
        cur_time.tm_mday = day;
        cur_time.tm_hour = hour;
        cur_time.tm_min = minute;
        cur_time.tm_sec = 0;
        cur_time.tm_isdst = 0;
        // TODO: not portable.
        std::time_t conv_time = timegm(&cur_time); // + time_offset_dir * time_offset * 1800;
        state.current_time.set(conv_time, typedCheckDataBoth(group));
    }
}

void addTmcMessage(RdsState& state, const TmcMessage& msg)
{
    LOG(ERROR) << "TMC message: duration = " << msg.duration <<
        ", diversion = " << msg.diversion <<
        ", neg_dir = " << msg.negative_direction <<
        ", ext = " << msg.extent <<
        ", event = " << msg.event <<
        ", loc = " << msg.location <<
        ", total extra groups = " << msg.total_extra_groups <<
        ", extra data size = " << msg.extra.size() <<
        ", pending messages = " << state.tmc_pending_messages.size();
}

void addSingleGroupTmcMessage(RdsState& state, const TmcMessage& msg, RdsCheckData check_data)
{
    if (check_data.status == RdsDecodingStatus::Valid)
    {
        state.tmc_pending_messages.erase(check_data.data);
        addTmcMessage(state, msg);
    }
    else
    {
        CHECK(check_data.status == RdsDecodingStatus::Corrected);
        auto it = state.tmc_pending_messages.find(check_data.data);
        if (it != state.tmc_pending_messages.end())
        {
            state.tmc_pending_messages.erase(it);
            addTmcMessage(state, msg);
        }
        else
        {
            state.tmc_pending_messages.emplace(check_data.data, TmcPendingMessage(check_data, msg));
        }
    }
}

void addMultiGroupTmcMessage(RdsState& state, const TmcMessage& msg, const TmcCheckData& check_data)
{
    if (check_data.status == RdsDecodingStatus::Valid)
    {
        state.tmc_pending_messages.erase(check_data.data[0]);
        addTmcMessage(state, msg);
    }
    else
    {
        CHECK(check_data.status == RdsDecodingStatus::Corrected);
        auto it = state.tmc_pending_messages.find(check_data.data[0]);
        if (it != state.tmc_pending_messages.end())
        {
            std::size_t matching_groups = 1;
            for (; matching_groups < std::min(it->second.check_data.data.size(), check_data.data.size()); ++matching_groups)
            {
                if (it->second.check_data.data[matching_groups] != check_data.data[matching_groups])
                {
                    break;
                }
            }

            TmcMessage merged_msg(msg);
            merged_msg.extra.resize(matching_groups);

            state.tmc_pending_messages.erase(it);
            addTmcMessage(state, merged_msg);
        }
        else
        {
            state.tmc_pending_messages.emplace(check_data.data[0], TmcPendingMessage(check_data, msg));
        }
    }
}

void flushTmcMultiGroup(RdsState& state, std::uint16_t continuity_index)
{
    auto first_it = std::max_element(
        state.tmc_multigroups_first[continuity_index].begin(),
        state.tmc_multigroups_first[continuity_index].end(),
        [](const auto& msg0, const auto& msg1)
        {
            return msg0.second.count < msg1.second.count;
        }
    );

    auto second_it = std::max_element(
        state.tmc_multigroups_second[continuity_index].begin(),
        state.tmc_multigroups_second[continuity_index].end(),
        [](const auto& msg0, const auto& msg1)
        {
            return msg0.second.count < msg1.second.count;
        }
    );

    if (first_it != state.tmc_multigroups_first[continuity_index].end() &&
        second_it != state.tmc_multigroups_second[continuity_index].end())
    {
        TmcMessage msg = first_it->second.message;
        TmcCheckData check_data;
        msg.total_extra_groups = second_it->second.remaining_groups + 1;
        msg.extra.push_back(second_it->second.data);
        check_data.data.push_back(first_it->first);
        check_data.data.push_back(second_it->first);

        std::uint16_t count = std::min(first_it->second.count, second_it->second.count);

        for (std::int16_t remaining = std::int16_t(second_it->second.remaining_groups) - 1; remaining >= 0; --remaining)
        {
            auto next_it = std::max_element(
                state.tmc_multigroups_next[continuity_index][remaining].begin(),
                state.tmc_multigroups_next[continuity_index][remaining].end(),
                [](const auto& msg0, const auto& msg1)
                {
                    return msg0.second.count < msg1.second.count;
                }
            );

            if (next_it == state.tmc_multigroups_next[continuity_index][remaining].end())
            {
                break;
            }

            msg.extra.push_back(next_it->second.data);
            check_data.data.push_back(next_it->first);
            count = std::min(count, next_it->second.count);
        }

        check_data.status = count >= 2 ? RdsDecodingStatus::Valid : RdsDecodingStatus::Corrected;

        addMultiGroupTmcMessage(state, msg, check_data);
    }

    // Regardless of the outcome, clear all the data for this continuity_index.
    state.tmc_multigroups_first[continuity_index].clear();
    state.tmc_multigroups_second[continuity_index].clear();
    for (auto& next: state.tmc_multigroups_next[continuity_index])
    {
        next.clear();
    }
}

void clearTmc(RdsState& state, bool everything = true, std::time_t cutoff_time = 0)
{
    for (std::uint16_t i = 0; i < TmcContinuityCount; ++i)
    {
        bool expired = everything;

        for (const auto& group: state.tmc_multigroups_first[i])
        {
            expired = expired || std::difftime(cutoff_time, group.second.timestamp) > TmcImmediateTimeout;
        }

        for (const auto& group: state.tmc_multigroups_second[i])
        {
            expired = expired || std::difftime(cutoff_time, group.second.timestamp) > TmcImmediateTimeout;
        }

        for (std::uint16_t j = 0; j < TmcMaxExtraGroupsCount; ++j)
        {
            for (const auto& group: state.tmc_multigroups_next[i][j])
            {
                expired = expired || std::difftime(cutoff_time, group.second.timestamp) > TmcImmediateTimeout;
            }
        }

        if (expired)
        {
            flushTmcMultiGroup(state, i);
        }
    }
}


void extractTmcMessage(TmcMessage& msg, const RdsGroup& group)
{
    msg.duration = group[1].data & ((1 << 3) - 1);
    msg.diversion = group[2].data & (1 << 15);
    msg.negative_direction = group[2].data & (1 << 14);
    msg.extent = (group[2].data >> 11) & ((1 << 3) - 1);
    msg.event = group[2].data & ((1 << 11) - 1);
    msg.location = group[3].data;
    msg.total_extra_groups = 0;
}

void decodeTmcSingleGroup(RdsState& state, const RdsGroup& group)
{
    TmcMessage msg;
    extractTmcMessage(msg, group);
    addSingleGroupTmcMessage(state, msg, typedCheckDataBoth(group));
}

void decodeTmcMultiGroup(RdsState& state, const RdsGroup& group)
{
    std::uint16_t continuity_index = (group[1].data & ((1 << 3) - 1)) - 1;
    CHECK(continuity_index < TmcContinuityCount);
    RdsCheckData check_data = tmcMultiGroupCheckData(group);

    if (group[2].data & (1 << 15))
    {
        // First group.
        TmcMessage msg;
        extractTmcMessage(msg, group);
        TmcFirstGroup& info = state.tmc_multigroups_first[continuity_index][check_data.data];
        info.timestamp = std::time(nullptr);
        info.count += (check_data.status == RdsDecodingStatus::Valid) ? 2 : 1;
        info.message = msg;
    }
    else
    {
        // Subsequent group.
        std::uint16_t remaining_groups = (group[2].data >> 12) & 0x03;
        bool second = group[2].data & (1 << 14);
        std::uint16_t count_inc = (check_data.status == RdsDecodingStatus::Valid) ? 2 : 1;
        std::uint32_t data = std::uint32_t((group[2].data & ((1 << 12) - 1)) << 16) | group[3].data;

        std::uint16_t total_count = 0;
        if (second)
        {
            TmcSecondGroup& info = state.tmc_multigroups_second[continuity_index][check_data.data];
            info.timestamp = std::time(nullptr);
            info.remaining_groups = remaining_groups;
            info.count += count_inc;
            info.data = data;
            total_count = info.count;
        }
        else
        {
            TmcNextGroup& info = state.tmc_multigroups_next[continuity_index][remaining_groups][check_data.data];
            info.timestamp = std::time(nullptr);
            info.count += count_inc;
            info.data = data;
            total_count = info.count;
        }

        if (!remaining_groups && total_count >= 2)
        {
            // This is valid end block for multi-group with given continuity index,
            // so we should not expect any further messages for previous blocks
            // for this continuity index within this transmission period. Therefore,
            // it's OK to flush it immediately, without waiting for the transmission
            // period expiration.
            flushTmcMultiGroup(state, continuity_index);
        }
    }
}

TmcChannel& ensureTmcChannel(RdsState& state, const RdsGroup& group, std::size_t index)
{
    std::uint16_t pi = group[index].data;
    auto it = std::find_if(
        state.tmc_channels.begin(),
        state.tmc_channels.end(),
        [pi](const auto& cur_channel)
        {
            return cur_channel.programme_identification.lastValue() == pi;
        }
    );

    if (it != state.tmc_channels.end())
    {
        it->programme_identification.set(pi, typedCheckData(group, index));
        return *it;
    }
    else
    {
        state.tmc_channels.emplace_back();
        state.tmc_channels.back().programme_identification.set(pi, typedCheckData(group, index));
        return state.tmc_channels.back();
    }
}

void decodeTmcTuningInformation(RdsState& state, const RdsGroup& group)
{
    std::uint16_t variant_code = group[1].data & ((1 << 4) - 1);
    CHECK(variant_code >= 0x04 && variant_code <= 0x09);

    switch (variant_code)
    {
        case 0x04:
        case 0x05:
            state.tmc_service_provider_name[(variant_code - 0x04) * 4].set(char(group[2].data >> 8), typedCheckData(group, 2));
            state.tmc_service_provider_name[(variant_code - 0x04) * 4 + 1].set(char(group[2].data & 0xFF), typedCheckData(group, 2));
            state.tmc_service_provider_name[(variant_code - 0x04) * 4 + 2].set(char(group[3].data >> 8), typedCheckData(group, 3));
            state.tmc_service_provider_name[(variant_code - 0x04) * 4 + 3].set(char(group[3].data & 0xFF), typedCheckData(group, 3));
            break;

        case 0x06:
        {
            TmcChannel& channel = ensureTmcChannel(state, group, 3);
            addFrequencies(channel.frequencies, group, 2);
            break;
        }

        case 0x07:
        {
            // TODO: ignoring tuned frequency here.
            TmcChannel& channel = ensureTmcChannel(state, group, 3);
            updateFrequency(
                channel.frequencies,
                extractFrequency(group[2].data & 0xFF),
                typedCheckData(group, 2)
            );
            break;
        }

        case 0x08:
            if (group[2].data)
            {
                ensureTmcChannel(state, group, 2);
            }

            if (group[3].data)
            {
                ensureTmcChannel(state, group, 3);
            }
            break;

        case 0x09:
        {
            TmcChannel& channel = ensureTmcChannel(state, group, 3);
            RdsCheckData check_data = typedCheckData(group, 2);
            channel.ltn.set(group[2].data >> 10, check_data);
            channel.scope_i13l.set(group[2].data & (1 << 9), check_data);
            channel.scope_national.set(group[2].data & (1 << 8), check_data);
            channel.scope_regional.set(group[2].data & (1 << 7), check_data);
            channel.scope_urban.set(group[2].data & (1 << 6), check_data);
            channel.sid.set(group[2].data & ((1 << 6) - 1), check_data);
            break;
        }
    }
}

void decodeTmc(RdsState& state, const RdsGroup& group)
{
    // While in theory it's possible to extract smth meaningful even from partial responses,
    // the benefit / cost ratio doesn't seem to be too attractive.
    if (group[2].status >= RdsDecodingStatus::Corrected &&
        group[3].status >= RdsDecodingStatus::Corrected)
    {
        std::uint16_t type = group[1].data & ((1 << 5) - 1);

        if (type >= 0x01 && type <= 0x06)
        {
            // Multi-group case:
            decodeTmcMultiGroup(state, group);
        }
        else if (type >= 0x08 && type <= 0x0F)
        {
            // Single-group case:
            decodeTmcSingleGroup(state, group);
        }
        else if (type >= 0x14 && type <= 0x19)
        {
            // Tuning information:
            decodeTmcTuningInformation(state, group);
        }
        else
        {
            if (group[1].status == RdsDecodingStatus::Valid)
            {
                LOG(ERROR) << "Unknown TMC message type: " << type;
            }
        }
    }
}

void decodeTmcSystemInfoCommon(RdsState& state, const RdsGroup& group, RdsCheckData& check_data)
{
    state.tmc_ltn.set((group[2].data >> 6) & ((1 << 6) - 1), check_data);
    state.tmc_afi.set(group[2].data & (1 << 5), check_data);
    state.tmc_mode.set(group[2].data & (1 << 4), check_data);
    state.tmc_scope_i13l.set(group[2].data & (1 << 3), check_data);
    state.tmc_scope_national.set(group[2].data & (1 << 2), check_data);
    state.tmc_scope_regional.set(group[2].data & (1 << 1), check_data);
    state.tmc_scope_urban.set(group[2].data & 1, check_data);
}

void decodeTmcSystemInformation(RdsState& state, const RdsGroup& group)
{
    std::uint16_t variant_code = group[2].data >> 14;
    RdsCheckData check_data = typedCheckData(group, 2);

    switch (variant_code)
    {
        case 0x00:
        {
            decodeTmcSystemInfoCommon(state, group, check_data);
            break;
        }

        case 0x01:
        {
            state.tmc_gap.set((group[2].data >> 12) & 0x03, check_data);
            state.tmc_sid.set((group[2].data >> 6) & ((1 << 6) - 1), check_data);
            if (state.tmc_mode.valid() && state.tmc_mode.value())
            {
                state.tmc_activity_time.set((group[2].data >> 4) & 0x03, check_data);
                state.tmc_window_time.set((group[2].data >> 2) & 0x03, check_data);
                state.tmc_delay_time.set(group[2].data & 0x03, check_data);
            }
            break;
        }

        default:
            // No other variants are currently defined.
            if (group[1].status == RdsDecodingStatus::Valid && group[2].status == RdsDecodingStatus::Valid)
            {
                LOG(ERROR) << "Variant code " << variant_code << " is unexpected for TMC ODA init message";
            }
            break;
    }
}

// See ETSI EN 301 700 standard.
void decodeDabReference(RdsState& state, const RdsGroup& group)
{
    bool es = group[1].data & (1 << 4);

    if (es)
    {
        // Service table, see 5.3.4.
        std::uint8_t variant_code = group[1].data & 0x0F;

        switch (variant_code)
        {
            case 0x00:
                state.dab_eid.set(group[2].data, typedCheckData(group, 2));
                break;

            case 0x01:
            {
                RdsCheckData check_data = typedCheckData(group, 2);
                state.dab_link_linkage_actuator.set(group[2].data & (1 << 14), check_data);
                state.dab_link_soft_hard.set(group[2].data & (1 << 13), check_data);
                state.dab_link_i13l_linkage_set.set(group[2].data & (1 << 12), check_data);
                state.dab_link_linkage_set_number.set(group[2].data & ((1 << 12) - 1), check_data);
                break;
            }

            default:
                // No other variants are currently specified.
                break;
        }

        state.dab_sid.set(group[3].data, typedCheckData(group, 3));
    }
    else
    {
        // Ensemble table, see 5.3.3.
        state.dab_mode.set((group[1].data >> 2) & 0x03, group[1]);
        state.dab_freq.set((std::uint32_t((group[1].data & 0x03) << 16) | group[2].data) * 16000, typedCheckData(group, 2));
        state.dab_eid.set(group[3].data, typedCheckData(group, 3));
    }
}

void decodeOpenDataApplicationInitMessage(RdsState& state, const RdsGroup& group)
{
    switch (group[3].data)
    {
        case 0xCD46:
        case 0xCD47:
            decodeTmcSystemInformation(state, group);
            break;

        default:
            // No other currently supported ODA services use this initial message in any way, so skip it.
            break;
    }
}

void decodeOpenDataApplication(RdsState& state, std::uint16_t group_type, const RdsGroup& group)
{
    CHECK(state.oda_aids.count(group_type));
    switch (state.oda_aids[group_type].value())
    {
        case 0x93:
            decodeDabReference(state, group);
            break;

        case 0xCD46:
        case 0xCD47:
            decodeTmc(state, group);
            break;

        default:
            if (state.oda_aids[group_type].valid())
            {
                LOG(ERROR) << "Unknown ODA AID: " << state.oda_aids[group_type].value();
            }
            break;
    }
}

void decodeGroupType10A(RdsState& state, const RdsGroup& group)
{
    state.ptn_ab.set(group[1].data & (1 << 4), group[1]);
    std::uint16_t seg = group[1].data & 0x01;

    CHECK_LT(4 * seg + 1, state.programme_type_name.size());
    state.programme_type_name[4 * seg].set(char((group[2].data >> 8) & 0xFF), typedCheckData(group, 2));
    state.programme_type_name[4 * seg + 1].set(char(group[2].data & 0xFF), typedCheckData(group, 2));

    CHECK_LT(4 * seg + 3, state.programme_type_name.size());
    state.programme_type_name[4 * seg + 2].set(char((group[3].data >> 8) & 0xFF), typedCheckData(group, 3));
    state.programme_type_name[4 * seg + 3].set(char(group[3].data & 0xFF), typedCheckData(group, 3));
}

void decodeGroupType14(RdsState& state, bool ver_b, const RdsGroup& group)
{
    if (group[3].status < RdsDecodingStatus::Corrected)
    {
        // We don't know PI for the other network, so there's no point in
        // decoding EON info - we don't know for what network the info is for.
        return;
    }

    std::uint16_t pi_on = group[3].data;
    EonProgrammeInfo& eon_info = state.eon_mapping[pi_on];

    if (!ver_b && group[2].status >= RdsDecodingStatus::Corrected)
    {
        CHECK(group[1].status >= RdsDecodingStatus::Corrected);
        eon_info.traffic_programme.set((group[1].data >> 4) & 0x01, group[1]);
        std::uint8_t variant_code = group[1].data & 0x0F;
        // The data implicitly depends on group[3], as it determines into which value
        // of the map it goes, so make it dependent on group[3] as well.
        RdsCheckData check_data = typedCheckDataBoth(group);

        switch (variant_code)
        {
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
                eon_info.programme_service_name.resize(8);
                eon_info.programme_service_name[2 * variant_code].set(char((group[2].data >> 8) & 0xFF), check_data);
                eon_info.programme_service_name[2 * variant_code + 1].set(char(group[2].data & 0xFF), check_data);
                break;

            case 0x04:
                addFrequencies(eon_info.frequencies, group, 2, true);
                break;

            case 0x05:
            case 0x06:
            case 0x07:
            case 0x08:
            {
                // TODO: this records only part of the mapping, i.e. skips the info which tuned
                // freq is mapped to which other frequency.
                //std::int32_t tuned_freq = extractFrequency(group[2].data >> 8);
                std::int32_t mapped_freq = extractFrequency(group[2].data & 0xFF);
                if (mapped_freq)
                {
                    updateFrequency(eon_info.frequencies, mapped_freq, check_data);
                }
                break;
            }

            case 0x09:
            {
                // TODO: this records only part of the mapping, i.e. skips the info which tuned
                // freq is mapped to which other frequency.
                //std::int32_t tuned_freq = extractFrequency(group[2].data >> 8);
                std::int32_t mapped_freq = extractFrequency(group[2].data & 0xFF, true);
                if (mapped_freq)
                {
                    updateFrequency(eon_info.frequencies, mapped_freq, check_data);
                }
                break;
            }

            case 0x0C:
            {
                eon_info.linkage_actuator.set(group[2].data & (1 << 15), check_data);
                eon_info.extended_generic.set(group[2].data & (1 << 14), check_data);
                eon_info.i13l_linkage_set.set(group[2].data & (1 << 13), check_data);
                eon_info.linkage_set_number.set(group[2].data & ((1 << 12) - 1), check_data);
                break;
            }

            case 0x0D:
                eon_info.programme_type.set(group[2].data >> 11, check_data);
                eon_info.traffic_announcement.set(group[2].data & 0x01, check_data);
                break;

            case 0x0E:
                decodeProgrammeItemStartTime(eon_info.programme_item_start_time, group, 2, true);
                break;

            case 0x0F:
                // According to 3.1.5.19, "Reserved for broadcasters use".
                break;

            default:
                if (group[1].status == RdsDecodingStatus::Valid)
                {
                    LOG(ERROR) << "Unknown variant code: " << int(variant_code);
                }
                break;
        }
    }
}

void decodeGroupType15B(RdsState& state, const RdsGroup& group)
{
    decodeGroupFieldsCommon(state, group, 2);
    decodeTuningInfoCommon(state, group, 3);
}

void decodeGroup(RdsState& state, const RdsGroup& group)
{
    uint16_t group_type = decodeGroupCommon(state, group);
    if (group[1].status >= RdsDecodingStatus::Corrected)
    {
        switch (group_type)
        {
            case 0x00:
            case 0x01:
            {
                decodeGroupType0(state, group_type == 0x01, group);
                break;
            }

            case 0x02:
            case 0x03:
            {
                decodeGroupType1(state, group_type == 0x03, group);
                break;
            }

            case 0x04:
            case 0x05:
            {
                decodeGroupType2(state, group_type == 0x05, group);
                break;
            }

            case 0x06:
            {
                decodeGroupType3A(state, group);
                break;
            }

            case 0x08:
            {
                decodeGroupType4A(state, group);
                break;
            }

            case 0x10:
            {
                if (state.oda_aids.count(group_type))
                {
                    decodeOpenDataApplication(state, group_type, group);
                }
                else
                {
                    // Legacy mapping for TMC, if ODA is not used.
                    decodeTmc(state, group);
                }
                break;
            }

            case 0x14:
            {
                decodeGroupType10A(state, group);
                break;
            }

            case 0x1C:
            case 0x1D:
            {
                decodeGroupType14(state, group_type == 0x1D, group);
                break;
            }

            case 0x1E:
                // According to 3.1.5.20, "this group type is not specified in this standard and is currently unavailable".
                if (group[1].status == RdsDecodingStatus::Valid)
                {
                    LOG(ERROR) << "Group type 0x1E is unexpected";
                }
                break;

            case 0x1F:
            {
                decodeGroupType15B(state, group);
                break;
            }

            default:
                // All other group types can be used for some outdated purposes
                // ("Radio Paging", really?), carry proprietary formats (e.g. TDC, in-house) or
                // used for ODA. Given I don't want to implement the first type and have
                // no specs for the second type - the only remaining thing is ODA.
                if (state.oda_aids.count(group_type))
                {
                    decodeOpenDataApplication(state, group_type, group);
                }
                break;
        }
    }
}

} // anonymous namespace

RdsMessagesDecoder::RdsMessagesDecoder()
{
    // Turn off input buffering.
    Base::inputState(0).setSuggestedSize(1);

    rds_state_.text_ab.changing().connect([this](const auto&) { invalidateContainer(rds_state_.radio_text); });
    rds_state_.ptn_ab.changing().connect([this](const auto&) { invalidateContainer(rds_state_.programme_type_name); });
}

RdsState RdsMessagesDecoder::state() const
{
    std::unique_lock<std::mutex> lock(state_mutex_);
    RdsState state(rds_state_);
    lock.unlock();
    std::time_t current_time = std::time(nullptr);
    state.clear(false, current_time);
    clearTmc(state, false, current_time);
    return state;
}

void RdsMessagesDecoder::reset()
{
    std::unique_lock<std::mutex> lock(state_mutex_);
    rds_state_.clear();
    clearTmc(rds_state_);
}

void RdsMessagesDecoder::process(const typename Base::Inputs& input, typename Base::Outputs& /* output */)
{
    const auto& input_data = std::get<0>(input);
    const std::size_t input_data_size = input_data.size();

    for (std::size_t input_index = 0; input_index < input_data_size; ++input_index)
    {
        const RdsGroup& group = input_data[input_index];
        std::unique_lock<std::mutex> lock(state_mutex_);
        std::time_t current_time = std::time(nullptr);
        rds_state_.clear(false, current_time);
        clearTmc(rds_state_, false, current_time);
        decodeGroup(rds_state_, group);

#ifdef PRINT_RDS_STATE
        rds_state_.dump();
#endif
    }

    input_data.advance(input_data_size);
}
