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

struct _snd_pcm;
typedef struct _snd_pcm snd_pcm_t;

namespace hvylya {
namespace filters {

class AlsaError: public core::ExceptionBase
{
  public:
    AlsaError(int error);

    AlsaError(const AlsaError& ex) = default;

    AlsaError(AlsaError&& ex) = default;
};

template <typename T, std::size_t ChannelsCount = 1>
class AlsaSink:
    public FilterGeneric<
        typename core::TypeDuplicator<T, ChannelsCount>::Type,
        core::TypeList<>
    >
{
  public:
    static_assert(ChannelsCount > 0, "AlsaSink requires at least one channel");

    typedef typename FilterBaseType<AlsaSink>::Type Base;

    AlsaSink(const char* device_name, std::size_t rate, std::size_t samples_delay);

    virtual ~AlsaSink() override;

    virtual void reset() override;

    virtual void process(const typename Base::Inputs& input, typename Base::Outputs& /* output */) override;

  private:
    snd_pcm_t* pcm_device_;
    std::size_t samples_delay_;
    bool delayed_;
};

} // namespace filters
} // namespace hvylya
