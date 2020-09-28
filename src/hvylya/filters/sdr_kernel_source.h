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

namespace hvylya {
namespace filters {

struct SdrKernelBuffer
{
    std::uint8_t* data;
    std::size_t size;
};

template <class T>
class SdrKernelSource:
    public FilterGeneric<
        core::TypeList<>,
        core::TypeList<std::complex<T>>
    >
{
  public:
    typedef typename FilterBaseType<SdrKernelSource>::Type Base;

    SdrKernelSource(const char* device_name);

    virtual ~SdrKernelSource() override;

    void startStreaming();

    void stopStreaming();

    void setSamplingRate(std::size_t sampling_rate);

    void setCenterFrequency(T center_freq);

    void setBandwidth(T bandwidth);

    void setGain(T gain);

    virtual void reset() override;

    virtual void process(const typename Base::Inputs& /* input */, typename Base::Outputs& output) override;

  private:
    std::vector<SdrKernelBuffer> buffers_;
    std::string device_name_;
    std::size_t current_buffer_index_, consumed_bytes_;
    timeval last_acquired_timestamp_;
    int device_;
    bool streaming_, acquired_buffer_;

    void acquireBuffer();

    void releaseBuffer(std::size_t buffer_index);

    void mapBuffers();

    void unmapBuffers();

    void ioctlRetry(unsigned long request, void* arg);
};

} // namespace filters
} // namespace hvylya
