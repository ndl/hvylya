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

#include <hvylya/filters/sdr_kernel_source.h>

#include <libv4l2.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/videodev2.h>

#define V4L2_PIX_FMT_SDR_U8 v4l2_fourcc('C', 'U', '0', '8') // unsigned 8-bit

using namespace hvylya::core;
using namespace hvylya::filters;

namespace {

const std::uint32_t MinBuffersCount = 8, BuffersCount = 16;

}

template <typename T>
SdrKernelSource<T>::SdrKernelSource(const char* device_name):
    device_name_(device_name),
    streaming_(false),
    acquired_buffer_(false)
{
    // Open device in blocking mode - we're not doing any async processing,
    // as we currently don't have polling support in the pipeline scheduler.
    device_ = open(device_name, O_RDWR, 0);
    if (device_ < 0)
    {
        THROW(IoError()) << fmt::format("Cannot open device {0}", device_name);
    }

    v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_SDR_CAPTURE;
    format.fmt.sdr.pixelformat = V4L2_PIX_FMT_SDR_U8;
    ioctlRetry(VIDIOC_S_FMT, &format);

    if (format.fmt.sdr.pixelformat != V4L2_PIX_FMT_SDR_U8)
    {
        THROW(IoError()) << 
            fmt::format(
                "Unexpected format {0}, device = {1}",
                format.fmt.sdr.pixelformat,
                device_name
            );
    }

    v4l2_requestbuffers buffers_request;
    memset(&buffers_request, 0, sizeof(buffers_request));
    buffers_request.count = BuffersCount;
    buffers_request.type = V4L2_BUF_TYPE_SDR_CAPTURE;
    buffers_request.memory = V4L2_MEMORY_MMAP;
    ioctlRetry(VIDIOC_REQBUFS, &buffers_request);

    CHECK_GE(buffers_request.count, MinBuffersCount);
    buffers_.resize(buffers_request.count);
}

template <typename T>
SdrKernelSource<T>::~SdrKernelSource()
{
    stopStreaming();
    close(device_);
}

template <typename T>
void SdrKernelSource<T>::setSamplingRate(std::size_t sampling_rate)
{
    v4l2_frequency freq;
    memset(&freq, 0, sizeof(freq));
    freq.tuner = 0;
    freq.type = V4L2_TUNER_ADC;
    freq.frequency = std::uint32_t(sampling_rate);
    ioctlRetry(VIDIOC_S_FREQUENCY, &freq);
}

template <typename T>
void SdrKernelSource<T>::setCenterFrequency(T center_freq)
{
    v4l2_frequency freq;
    memset(&freq, 0, sizeof(freq));
    freq.tuner = 1;
    freq.type = V4L2_TUNER_RF;
    freq.frequency = std::uint32_t(center_freq);
    ioctlRetry(VIDIOC_S_FREQUENCY, &freq);
}

template <typename T>
void SdrKernelSource<T>::setBandwidth(T bandwidth)
{
    v4l2_ext_control ext_ctrl;
    memset(&ext_ctrl, 0, sizeof(ext_ctrl));
    ext_ctrl.id = V4L2_CID_RF_TUNER_BANDWIDTH;
    ext_ctrl.value = std::int32_t(bandwidth);

    v4l2_ext_controls ext_ctrls;
    memset(&ext_ctrls, 0, sizeof(ext_ctrls));
    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ext_ctrls.count = 1;
    ext_ctrls.controls = &ext_ctrl;

    ioctlRetry(VIDIOC_S_EXT_CTRLS, &ext_ctrls);
}

template <typename T>
void SdrKernelSource<T>::setGain(T gain)
{
    v4l2_ext_control ext_ctrl;
    memset(&ext_ctrl, 0, sizeof(ext_ctrl));
    ext_ctrl.id = V4L2_CID_RF_TUNER_IF_GAIN;
    ext_ctrl.value = std::int32_t(gain);

    v4l2_ext_controls ext_ctrls;
    memset(&ext_ctrls, 0, sizeof(ext_ctrls));
    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_USER;
    ext_ctrls.count = 1;
    ext_ctrls.controls = &ext_ctrl;

    ioctlRetry(VIDIOC_S_EXT_CTRLS, &ext_ctrls);
}

template <typename T>
void SdrKernelSource<T>::process(const typename Base::Inputs& /* input */, typename Base::Outputs& output)
{
    if (!streaming_)
    {
        startStreaming();
    }

    if (!acquired_buffer_)
    {
        acquireBuffer();
    }

    auto& output_data = std::get<0>(output);
    std::size_t output_size = std::min(Base::outputState(0).suggestedSize(), output_data.size());

    std::size_t output_index = 0;
    while (output_index < output_size)
    {
        // Values should be delivered & consumed in pairs, I and Q.
        CHECK_EQ(0, consumed_bytes_ % 2);

        const SdrKernelBuffer& buffer(buffers_[current_buffer_index_]);
        for (; consumed_bytes_ < buffer.size && output_index < output_size; ++output_index, consumed_bytes_ += 2)
        {
            output_data[output_index] =
                std::complex<T>(
                    (buffer.data[consumed_bytes_] - T(127.5)) / T(127.5),
                    (buffer.data[consumed_bytes_ + 1] - T(127.5)) / T(127.5)
                );
        }

        if (consumed_bytes_ == buffer.size)
        {
            CHECK(acquired_buffer_);
            releaseBuffer(current_buffer_index_);
        }

        if (output_index < output_size)
        {
            CHECK(!acquired_buffer_);
            acquireBuffer();
        }
    }

    output_data.advance(output_index);
}

template <typename T>
void SdrKernelSource<T>::startStreaming()
{
    CHECK(!streaming_);
    mapBuffers();
    v4l2_buf_type type = V4L2_BUF_TYPE_SDR_CAPTURE;
    ioctlRetry(VIDIOC_STREAMON, &type);
    streaming_ = true;
}

template <typename T>
void SdrKernelSource<T>::stopStreaming()
{
    CHECK(streaming_);
    v4l2_buf_type type = V4L2_BUF_TYPE_SDR_CAPTURE;
    ioctlRetry(VIDIOC_STREAMOFF, &type);
    unmapBuffers();
    acquired_buffer_ = false;
    streaming_ = false;
}

template <typename T>
void SdrKernelSource<T>::reset()
{
    Base::reset();

    if (!streaming_)
    {
        return;
    }

    if (!acquired_buffer_)
    {
        acquireBuffer();
    }

    timespec current_timestamp;
    CHECK_EQ(0, clock_gettime(CLOCK_MONOTONIC, &current_timestamp));

    while (
        last_acquired_timestamp_.tv_sec < current_timestamp.tv_sec ||
        (last_acquired_timestamp_.tv_sec == current_timestamp.tv_sec &&
         last_acquired_timestamp_.tv_usec * 1000 < current_timestamp.tv_nsec)
    )
    {
        releaseBuffer(current_buffer_index_);
        acquireBuffer();
    }
}

template <typename T>
void SdrKernelSource<T>::acquireBuffer()
{
    v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_SDR_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    ioctlRetry(VIDIOC_DQBUF, &buffer);

    CHECK_EQ(0, buffer.bytesused % 2);
    // V4L interface doesn't define any other constants for clock types, so if this one
    // is not set - we're screwed.
    CHECK_EQ(V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC, buffer.flags & V4L2_BUF_FLAG_TIMESTAMP_MASK);

    current_buffer_index_ = buffer.index;
    consumed_bytes_ = 0;
    buffers_[current_buffer_index_].size = buffer.bytesused;
    last_acquired_timestamp_ = buffer.timestamp;
    acquired_buffer_ = true;
}

template <typename T>
void SdrKernelSource<T>::releaseBuffer(std::size_t buffer_index)
{
    v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_SDR_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = std::uint32_t(buffer_index);
    ioctlRetry(VIDIOC_QBUF, &buffer);
    acquired_buffer_ = false;
}

template <typename T>
void SdrKernelSource<T>::mapBuffers()
{
    for (std::size_t buffer_index = 0; buffer_index < buffers_.size(); ++buffer_index)
    {
        v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_SDR_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = std::uint32_t(buffer_index);
        ioctlRetry(VIDIOC_QUERYBUF, &buffer);

        buffers_[buffer_index].size = buffer.length;
        buffers_[buffer_index].data =
            reinterpret_cast<std::uint8_t*>(
                v4l2_mmap(
                    nullptr,
                    buffer.length,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    device_,
                    buffer.m.offset
                )
            );
        if (buffers_[buffer_index].data == MAP_FAILED)
        {
            THROW(IoError()) << fmt::format("mmap failure, device = {0}", device_name_);
        }
    }

    for (std::size_t buffer_index = 0; buffer_index < buffers_.size(); ++buffer_index)
    {
        releaseBuffer(buffer_index);
    }
}

template <typename T>
void SdrKernelSource<T>::unmapBuffers()
{
    for (auto& buffer: buffers_)
    {
        v4l2_munmap(buffer.data, buffer.size);
    }
}

template <typename T>
void SdrKernelSource<T>::ioctlRetry(unsigned long request, void* arg)
{
    int result = 0;

    do
    {
        errno = 0;
        result = v4l2_ioctl(device_, request, arg);
    }
    while (result == -1 && (errno == EINTR || errno == EAGAIN));

    if (result == -1)
    {
        THROW(IoError()) << 
            fmt::format(
                "ioctl failure for request {0}, device = {1}",
                request,
                device_name_
            );
    }
}

template class hvylya::filters::SdrKernelSource<float>;
