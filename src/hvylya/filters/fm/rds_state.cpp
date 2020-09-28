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

#include <hvylya/filters/fm/rds_state.h>
#include <hvylya/filters/fm/rds_group.h>
#include <iostream>

using namespace hvylya::filters;

namespace {

std::time_t validity(ValueTag tag)
{
    switch (tag)
    {
        // This tag is for rarely changing information that is
        // constant for any given programme, such as program type,
        // language, etc.
        case ValueTag::Programme:
            return 60;

        // Clock has max error of a minute, so by having 60 secs validity
        // we're not increasing this max error.
        case ValueTag::Clock:
            return 60;

        // Text blocks are transmitted not very frequently and
        // might take some time to accumulate, so use relatively
        // high value here.
        case ValueTag::Text:
            return 30;

        // According to Table 4 in 3.1.3, most properties
        // that we mark with this validity tag are repeated
        // at least once per second, but we use higher value
        // to account for possible failed blocks.
        case ValueTag::Current:
            return 15;
    }
}

template <typename T, ValueTag Tag>
bool expired(const RdsValue<T, Tag>& value, bool everything, std::time_t cutoff_time)
{
    return everything || std::difftime(cutoff_time, value.lastUpdate()) > validity(Tag);
}

template <typename T, ValueTag Tag>
bool clearValue(RdsValue<T, Tag>& value, bool everything, std::time_t cutoff_time)
{
    if (expired(value, everything, cutoff_time))
    {
        value.invalidate();
    }

    return !value.valid();
}

template <typename Container>
bool clearContainer(Container& container, bool everything, std::time_t cutoff_time);

template <typename T, typename Alloc>
bool clearContainerAndRemove(std::vector<T, Alloc>& container, bool everything, std::time_t cutoff_time);

template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
bool clearContainerAndRemove(std::unordered_map<Key, T, Hash, KeyEqual, Allocator>& container, bool everything, std::time_t cutoff_time);

bool clearValue(EonProgrammeInfo& value, bool everything, std::time_t cutoff_time)
{
    bool cleared = true;
    cleared = clearContainer(value.programme_service_name, everything, cutoff_time) && cleared;
    cleared = clearContainerAndRemove(value.frequencies, everything, cutoff_time) && cleared;
    cleared = clearValue(value.traffic_programme, everything, cutoff_time) && cleared;
    cleared = clearValue(value.traffic_announcement, everything, cutoff_time) && cleared;
    cleared = clearValue(value.programme_type, everything, cutoff_time) && cleared;
    cleared = clearValue(value.programme_item_start_time, everything, cutoff_time) && cleared;
    cleared = clearValue(value.linkage_actuator, everything, cutoff_time) && cleared;
    cleared = clearValue(value.extended_generic, everything, cutoff_time) && cleared;
    cleared = clearValue(value.i13l_linkage_set, everything, cutoff_time) && cleared;
    cleared = clearValue(value.linkage_set_number, everything, cutoff_time) && cleared;
    return cleared;
}

bool clearValue(TmcChannel& value, bool everything, std::time_t cutoff_time)
{
    bool cleared = true;
    cleared = clearContainerAndRemove(value.frequencies, everything, cutoff_time) && cleared;
    cleared = clearValue(value.programme_identification, everything, cutoff_time) && cleared;
    cleared = clearValue(value.ltn, everything, cutoff_time) && cleared;
    cleared = clearValue(value.sid, everything, cutoff_time) && cleared;
    cleared = clearValue(value.scope_i13l, everything, cutoff_time) && cleared;
    cleared = clearValue(value.scope_national, everything, cutoff_time) && cleared;
    cleared = clearValue(value.scope_regional, everything, cutoff_time) && cleared;
    cleared = clearValue(value.scope_urban, everything, cutoff_time) && cleared;
    return cleared;
}

template <typename Container>
bool clearContainer(Container& container, bool everything, std::time_t cutoff_time)
{
    std::size_t invalidations = 0;

    for (auto& value: container)
    {
        if (clearValue(value, everything, cutoff_time))
        {
            ++invalidations;
        }
    }

    return container.size() == invalidations;
}

template <typename T, typename Alloc>
bool clearContainerAndRemove(std::vector<T, Alloc>& container, bool everything, std::time_t cutoff_time)
{
    container.erase(
        std::remove_if(
            container.begin(),
            container.end(),
            [everything, cutoff_time](auto& value)
            {
                return clearValue(value, everything, cutoff_time);
            }
        ),
        container.end()
    );

    return container.empty();
}

template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
bool clearContainerAndRemove(std::unordered_map<Key, T, Hash, KeyEqual, Allocator>& container, bool everything, std::time_t cutoff_time)
{
    for (auto it = container.begin(); it != container.end();)
    {
        if (clearValue(it->second, everything, cutoff_time))
        {
            auto tmp_it = it; ++it;
            container.erase(tmp_it);
        }
        else
        {
            ++it;
        }
    }

    return container.empty();
}

template <typename Container>
bool fillString(std::string& str, const Container& container, const char fillCharacter = '?')
{
    bool has_chars = false;

    for (const auto& value: container)
    {
        if (value.valid())
        {
            str.append(1, value.value());
            has_chars = true;
        }
        else
        {
            str.append(1, fillCharacter);
        }
    }

    return has_chars;
}

} // anonymous namespace

RdsCheckData::RdsCheckData()
{
}

RdsCheckData::RdsCheckData(std::uint64_t check_data, RdsDecodingStatus check_status):
    data(check_data),
    status(check_status)
{
}

RdsCheckData::RdsCheckData(const RdsBlock& block):
    data(block.data),
    status(block.status)
{
}

RdsState::RdsState()
{
    programme_service_name.resize(8);
    programme_type_name.resize(8);
    radio_text.resize(64);
    tmc_service_provider_name.resize(8);
}

void RdsState::clear(bool everything, std::time_t cutoff_time)
{
    // For vector and map containers we need to perform custom processing
    // as it's not enough to just invalidate the values - we need
    // to actually remove them.
    clearContainerAndRemove(alternative_frequencies, everything, cutoff_time);
    clearContainerAndRemove(oda_aids, everything, cutoff_time);
    clearContainerAndRemove(transparent_data_channels, everything, cutoff_time);
    clearContainerAndRemove(eon_mapping, everything, cutoff_time);
    clearContainerAndRemove(tmc_channels, everything, cutoff_time);

    // For everything else - just use the standard clear routine.
    clearValue(programme_identification, everything, cutoff_time);
    clearValue(programme_type, everything, cutoff_time);
    clearValue(traffic_programme, everything, cutoff_time);

    clearValue(traffic_announcement, everything, cutoff_time);
    clearValue(music_speech, everything, cutoff_time);
    clearValue(stereo, everything, cutoff_time);
    clearValue(artificial_head, everything, cutoff_time);
    clearValue(compressed, everything, cutoff_time);
    clearValue(dynamic_pty, everything, cutoff_time);
    clearContainer(programme_service_name, everything, cutoff_time);

    clearValue(country, everything, cutoff_time);
    clearValue(language, everything, cutoff_time);
    clearValue(programme_item_start_time, everything, cutoff_time);
    clearValue(linkage_actuator, everything, cutoff_time);

    clearContainer(radio_text, everything, cutoff_time);
    clearValue(text_ab, everything, cutoff_time);

    clearValue(oda_message, everything, cutoff_time);

    clearValue(current_time, everything, cutoff_time);

    clearContainer(programme_type_name, everything, cutoff_time);
    clearValue(ptn_ab, everything, cutoff_time);

    clearValue(dab_eid, everything, cutoff_time);
    clearValue(dab_sid, everything, cutoff_time);
    clearValue(dab_mode, everything, cutoff_time);
    clearValue(dab_link_linkage_set_number, everything, cutoff_time);
    clearValue(dab_link_linkage_actuator, everything, cutoff_time);
    clearValue(dab_link_soft_hard, everything, cutoff_time);
    clearValue(dab_link_extended_generic, everything, cutoff_time);
    clearValue(dab_link_i13l_linkage_set, everything, cutoff_time);

    clearContainer(tmc_service_provider_name, everything, cutoff_time);
    clearValue(tmc_ltn, everything, cutoff_time);
    clearValue(tmc_gap, everything, cutoff_time);
    clearValue(tmc_sid, everything, cutoff_time);
    clearValue(tmc_activity_time, everything, cutoff_time);
    clearValue(tmc_window_time, everything, cutoff_time);
    clearValue(tmc_delay_time, everything, cutoff_time);
    clearValue(tmc_afi, everything, cutoff_time);
    clearValue(tmc_mode, everything, cutoff_time);
    clearValue(tmc_scope_i13l, everything, cutoff_time);
    clearValue(tmc_scope_national, everything, cutoff_time);
    clearValue(tmc_scope_regional, everything, cutoff_time);
    clearValue(tmc_scope_urban, everything, cutoff_time);
}

void RdsState::dump() const
{
    if (programme_identification.valid())
    {
        std::cout << std::hex << std::uppercase << "PI: 0x" << programme_identification.value();

	if (programme_type.valid())
	{
            std::cout << ", PTY: 0x" << programme_type.value() << std::dec << ", TP: " << int(traffic_programme.value());
	}

        if (traffic_announcement.valid())
        {
            std::cout << ", TA: " << int(traffic_announcement.value());
	}

	if (music_speech.valid())
	{
            std::cout << ", MS: " << int(music_speech.value());
	}

	if (stereo.valid())
	{
            std::cout << ", Stereo: " << int(stereo.value());
	}

	if (artificial_head.valid())
	{
            std::cout << ", AH: " << int(artificial_head.value());
	}

	if (compressed.valid())
	{
            std::cout << ", CMP: " << int(compressed.value());
	}

	if (dynamic_pty.valid())
	{
            std::cout << ", DPTY: " << int(dynamic_pty.value());
        }

        std::string psn;
        if (fillString(psn, programme_service_name))
        {
            std::cout << ", PS: '" << psn << "'";
        }

        std::cout << std::dec << std::endl;

        if (!alternative_frequencies.empty())
        {
            std::cout << "AFs: ";
            for (std::size_t i = 0; i < alternative_frequencies.size(); ++i)
            {
                if (i > 0) std::cout << ", ";
                std::cout << alternative_frequencies[i].value();
            }
            std::cout << std::endl;
        }

        if (linkage_actuator.valid())
        {
            std::cout << "LA: " << linkage_actuator.value();

            if (country.valid())
            {
                std::cout << ", country: " << country.value();
            }

            if (language.valid())
            {
                std::cout << ", language: " << language.value();
            }

            std::cout << std::endl;
        }

        if (programme_item_start_time.valid() &&
            programme_item_start_time.value().day_of_month)
        {
            std::cout << "PIN: " <<
                fmt::format(
                    "{0:0>2d} {1:0>2d}:{2:0>2d}",
                    programme_item_start_time.value().day_of_month,
                    programme_item_start_time.value().hour,
                    programme_item_start_time.value().minute
                ) << std::endl;
        }

        std::string rt;
        if (fillString(rt, radio_text))
        {
            std::cout << "RT: " << rt << std::endl;
        }

        if (!oda_aids.empty())
        {
            std::cout << "ODA AIDS: " << std::hex;
            bool first = true;
            for (const auto& kv: oda_aids)
            {
                if (kv.second.valid())
                {
                    if (!first) std::cout << ", ";
                    first = false;
                    std::cout << "0x" << kv.first << " -> 0x" << kv.second.value();
                }
            }
            std::cout << std::dec << std::endl;
        }

        if (!eon_mapping.empty())
        {
            std::cout << "EON mapping: ";
            bool first = true;
            for (const auto& kv: eon_mapping)
            {
                if (!first) std::cout << "; ";
                first = false;

                std::string on_psn;
                fillString(on_psn, kv.second.programme_service_name);
                std::cout << "0x" << std::hex << kv.first << " -> " << on_psn;

                if (kv.second.programme_type.valid())
                {
                    std::cout << ", PTY: 0x" << kv.second.programme_type.value();
                }

                if (kv.second.traffic_programme.valid())
                {
                    std::cout << ", TP: " << kv.second.traffic_programme.value();
                }

                if (kv.second.traffic_announcement.valid())
                {
                    std::cout << ", TA: " << kv.second.traffic_announcement.value();
                }

                std::cout << std::dec;

                if (kv.second.programme_item_start_time.valid() &&
                    kv.second.programme_item_start_time.value().day_of_month)
                {
                        std::cout << ", PIN: " <<
                            fmt::format(
                                "{0:0>2d} {1:0>2d}:{2:0>2d}",
                                kv.second.programme_item_start_time.value().day_of_month,
                                kv.second.programme_item_start_time.value().hour,
                                kv.second.programme_item_start_time.value().minute
                            ) << std::endl;
                }

                if (kv.second.frequencies.size())
                {
                    std::cout << ", Freqs: (";
                    bool first_freq = true;
                    for (std::size_t i = 0; i < kv.second.frequencies.size(); ++i)
                    {
                        if (kv.second.frequencies[i].valid())
                        {
                            if (!first_freq) std::cout << ", ";
                            first_freq = false;
                            std::cout << kv.second.frequencies[i].value();
                        }
                    }
                    std::cout << ")"; 
                }

                if (kv.second.linkage_actuator.valid())
                {
                    std::cout << ", LA: " << kv.second.linkage_actuator.value() <<
                        ", EG: " << kv.second.extended_generic.value() <<
                        ", ILS: " << kv.second.i13l_linkage_set.value() <<
                        ", LSN: 0x" << std::hex << kv.second.linkage_set_number.value() <<
                        std::dec;
                }
            }
            std::cout << std::endl;
        }

        if (current_time.valid() && current_time.value())
        {
            struct tm* cur_time = std::localtime(&static_cast<const std::time_t&>(current_time.value()));
            char cur_time_str[128];
            std::strftime(cur_time_str, sizeof(cur_time_str), "%Y-%m-%d %H:%M %Z", cur_time);
            std::cout << "Time: " << cur_time_str << std::endl;
        }

        std::string ptn;
        if (fillString(ptn, programme_type_name))
        {
            std::cout << "PTN: " << ptn << std::endl;
        }

        std::string tmc_spn;
        if (fillString(tmc_spn, tmc_service_provider_name))
        {
            std::cout << "TMC SPN: " << tmc_spn << std::endl;
        }

        if (tmc_ltn.valid())
        {
            std::cout << "TMC LTN: 0x" << std::hex << tmc_ltn.value() <<
                ", AFI: " << tmc_afi.value() <<
                ", Mode: " << tmc_mode.value() <<
                ", I-scope: " << tmc_scope_i13l.value() <<
                ", N-scope: " << tmc_scope_national.value() <<
                ", R-scope: " << tmc_scope_regional.value() <<
                ", U-scope: " << tmc_scope_urban.value();

            if (tmc_gap.valid())
            {
                std::cout << ", Gap: " << tmc_gap.value() <<
                    ", SID: 0x" << tmc_sid.value();

                if (tmc_activity_time.valid())
                {
                    std::cout << ", Ta: " << tmc_activity_time.value() <<
                        ", Tw: " << tmc_window_time.value() <<
                        ", Td: " << tmc_delay_time.value();
                }
            }

            std::cout << std::dec << std::endl;
        }

        if (!tmc_channels.empty())
        {
            std::cout << "TMC channels: " << std::hex;
            bool first = true;
            for (const auto& channel: tmc_channels)
            {
                if (channel.programme_identification.valid())
                {
                    if (!first) std::cout << "; ";
                    first = false;

                    std::cout << "PI: 0x" << channel.programme_identification.value();
                    if (channel.ltn.valid())
                    {
                        std::cout << ", LTN: 0x" << channel.ltn.value() <<
                            ", SID: 0x" << channel.sid.value() <<
                            ", I-scope: " << channel.scope_i13l.value() <<
                            ", N-scope: " << channel.scope_national.value() <<
                            ", R-scope: " << channel.scope_regional.value() <<
                            ", U-scope: " << channel.scope_urban.value();
                    }

                    if (!channel.frequencies.empty())
                    {
                        std::cout << ", Freqs: (";
                        bool first_freq = true;
                        for (std::size_t i = 0; i < channel.frequencies.size(); ++i)
                        {
                            if (channel.frequencies[i].valid())
                            {
                                if (!first_freq) std::cout << ", ";
                                first_freq = false;
                                std::cout << channel.frequencies[i].value();
                            }
                        }
                        std::cout << ")"; 
                    }
                }
            }

            std::cout << std::dec << std::endl;
        }

        if (dab_eid.valid())
        {
            std::cout << "DAB EID: 0x" << std::hex << dab_eid.value();

            if (dab_sid.valid())
            {
                std::cout << ", SID: 0x" << dab_sid.value();
            }

            if (dab_link_linkage_actuator.valid())
            {
                std::cout << ", LA: " << dab_link_linkage_actuator.value() <<
                    ", S/H: " << dab_link_soft_hard.value() <<
                    ", ILS: " << dab_link_i13l_linkage_set.value() <<
                    ", LSN: 0x" << dab_link_linkage_set_number.value();
            }

            if (dab_mode.valid())
            {
                std::cout << ", Mode: " << dab_mode.value();
            }

            std::cout << std::dec;

            if (dab_freq.valid())
            {
                std::cout << ", Freq: " << dab_freq.value();
            }

            std::cout << std::endl;
        }

        std::cout << std::endl;
    }
}
