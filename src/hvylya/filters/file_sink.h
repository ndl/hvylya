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

#include <fstream>

namespace hvylya {
namespace filters {

template <typename T, std::size_t ChannelsCount = 1>
class FileSink:
    public FilterGeneric<
        typename core::TypeDuplicator<T, ChannelsCount>::Type,
        core::TypeList<>
    >
{
  public:
    static_assert(ChannelsCount > 0, "FileSink requires at least one channel");

    typedef typename FilterBaseType<FileSink>::Type Base;

    FileSink(const char* file_path):
        file_(file_path, std::ios::out | std::ios::binary),
        file_path_(file_path)
    {
        if (!file_)
        {
            THROW(core::IoError()) << fmt::format("Failed to open the file {0} for writing", file_path_);
        }
    }

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& /* output */) override
    {
        std::size_t input_data_size = std::numeric_limits<std::size_t>::max();
        forEachTupleElement(
            input,
            [&input_data_size](const auto& channel, auto /* channel_index */)
            {
                input_data_size = std::min(input_data_size, channel.size());
            }
        );

        data_.resize(ChannelsCount * input_data_size);

        // Interleave channels.
        forEachTupleElement(
            input,
            [=](const auto& channel, auto channel_index)
            {
                for (std::size_t i = 0; i < input_data_size; ++i)
                {
                    data_[ChannelsCount * i + channel_index] = channel[i];
                }

                channel.advance(input_data_size);
            }
        );

        file_.write(reinterpret_cast<const char*>(&data_[0]), static_cast<std::streamsize>(data_.size() * sizeof(T)));
        if (!file_)
        {
            THROW(core::IoError()) << fmt::format("Failed to write to the file {0}", file_path_);
        }
    }

  private:
    std::ofstream file_;
    std::string file_path_;
    std::vector<T> data_;
};

} // namespace filters
} // namespace hvylya
