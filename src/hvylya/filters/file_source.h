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

template <class T>
class FileSource:
    public FilterGeneric<
        core::TypeList<>,
        core::TypeList<T>
    >
{
  public:
    typedef typename FilterBaseType<FileSource>::Type Base;

    FileSource(const char* file_path, bool loop = false):
        file_(file_path, std::ios::in | std::ios::binary),
        file_path_(file_path),
        loop_(loop)
    {
        if (!file_)
        {
            THROW(core::IoError()) << fmt::format("Failed to open the file {0} for reading", file_path_);
        }
    }

    virtual void process(const typename Base::Inputs& /* input */, typename Base::Outputs& output) override
    {
        auto& output_data = std::get<0>(output);
        std::size_t output_size = std::min(Base::outputState(0).suggestedSize(), output_data.size());

        std::size_t read_data =
            std::size_t(
                file_.read(
                    reinterpret_cast<char*>(&output_data[0]),
                    std::streamsize(output_size * sizeof(T))
                ).gcount()
            );

        if (!file_)
        {
            if (!file_.eof())
            {
                THROW(core::IoError()) << fmt::format("Error while reading the file {0}", file_path_);
            }

            if (loop_)
            {
                file_.clear();
                file_.seekg(0, std::ios::beg);
                read_data =
                    std::size_t(
                        file_.read(
                            reinterpret_cast<char*>(&output_data[0]),
                            std::streamsize(output_size * sizeof(T))
                        ).gcount()
                    );
            }
        }

        if (read_data < sizeof(T))
        {
            Base::outputState(0).setEof(true);
        }
        else
        {
            output_data.advance(read_data / sizeof(T));
        }
    }

  private:
    std::ifstream file_;
    std::string file_path_;
    bool loop_;
};

} // namespace filters
} // namespace hvylya
