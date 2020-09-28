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

#include <hvylya/filters/alsa_sink.h>

#include <alsa/asoundlib.h>

using namespace hvylya::core;
using namespace hvylya::filters;

namespace {

template <typename SampleType>
struct AlsaTraits
{
};

template <>
struct AlsaTraits<float>
{
    static const snd_pcm_format_t PcmFormat = SND_PCM_FORMAT_FLOAT_LE;
};

void recoverFromUnderrun(snd_pcm_t* pcm_device)
{
    LOG(ERROR) << "ALSA underrun detected, recovering";
    snd_pcm_recover(pcm_device, -EPIPE, 0);
}

std::size_t availableFrames(snd_pcm_t* pcm_device)
{
    while (true)
    {
        snd_pcm_sframes_t avail_frames = snd_pcm_avail_update(pcm_device);

        if (avail_frames == -EPIPE)
        {
            recoverFromUnderrun(pcm_device);
        }
        else if (avail_frames < 0)
        {
            THROW(AlsaError(int(avail_frames))) << "Alsa error when determining the available buffer size";
        }
        else
        {
            return std::size_t(avail_frames);
        }
    }
}

} // anonymous namespace

AlsaError::AlsaError(int error)
{
    setDetails(snd_strerror(error));
}

template <typename T, std::size_t ChannelsCount>
AlsaSink<T, ChannelsCount>::AlsaSink(const char* device_name, std::size_t rate, std::size_t samples_delay):
    samples_delay_(samples_delay),
    delayed_(false)
{
    snd_pcm_hw_params_t* hw_params;
    int error = 0;

    if ((error = snd_pcm_open(&pcm_device_, device_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        THROW(AlsaError(error)) << fmt::format("Cannot open sound device {0}", device_name);
    }

    if ((error = snd_pcm_hw_params_malloc(&hw_params)) < 0)
    {
        THROW(AlsaError(error)) << "Cannot allocate hardware parameter structure";
    }

    if ((error = snd_pcm_hw_params_any(pcm_device_, hw_params)) < 0)
    {
        THROW(AlsaError(error)) << "Cannot initialize hardware parameter structure";
    }

    if ((error = snd_pcm_hw_params_set_access(pcm_device_, hw_params, SND_PCM_ACCESS_RW_NONINTERLEAVED)) < 0)
    {
        THROW(AlsaError(error)) << fmt::format("Cannot set access type {0}", SND_PCM_ACCESS_RW_NONINTERLEAVED);
    }

    snd_pcm_format_t requested_format = AlsaTraits<T>::PcmFormat;
    if ((error = snd_pcm_hw_params_set_format(pcm_device_, hw_params, requested_format)) < 0)
    {
        THROW(AlsaError(error)) << fmt::format("Cannot set sample format {0}", requested_format);
    }

    unsigned requested_rate = unsigned(rate), actual_rate = requested_rate;
    if ((error = snd_pcm_hw_params_set_rate_near(pcm_device_, hw_params, &actual_rate, nullptr)) < 0)
    {
        THROW(AlsaError(error)) << fmt::format("Cannot set sample rate {0}", requested_rate);
    }
    if (actual_rate != requested_rate)
    {
        THROW(AlsaError(error)) << fmt::format("Failed to set sample rate {0}, got {1} instead", requested_rate, actual_rate);
    }

    if ((error = snd_pcm_hw_params_set_channels(pcm_device_, hw_params, ChannelsCount)) < 0)
    {
        THROW(AlsaError(error)) << fmt::format("Cannot set channels count {0}", ChannelsCount);
    }

    if ((error = snd_pcm_hw_params(pcm_device_, hw_params)) < 0)
    {
        THROW(AlsaError(error)) << "Cannot set parameters";
    }

    snd_pcm_hw_params_free(hw_params);

    // AlsaSink delays samples to create the safety margin for the scheduler /
    // the rest of pipeline, hence we won't be consuming anything at the beginning.
    for (std::size_t i = 0; i < ChannelsCount; ++i)
    {
        Base::inputState(i).setMayConsumeNothing(true);
    }
}

template <typename T, std::size_t ChannelsCount>
AlsaSink<T, ChannelsCount>::~AlsaSink()
{
    snd_pcm_close(pcm_device_);
}

template <typename T, std::size_t ChannelsCount>
void AlsaSink<T, ChannelsCount>::reset()
{
    Base::reset();
    delayed_ = false;
    int error = 0;
    if ((error = snd_pcm_drop(pcm_device_)) < 0)
    {
        THROW(AlsaError(error)) << "Cannot stop";
    }
}

template <typename T, std::size_t ChannelsCount>
void AlsaSink<T, ChannelsCount>::process(const typename Base::Inputs& input, typename Base::Outputs& /* output */)
{
    std::size_t input_data_size = std::numeric_limits<std::size_t>::max();
    forEachTupleElement(
        input,
        [&input_data_size](const auto& channel, auto /* channel_index */)
        {
            input_data_size = std::min(input_data_size, channel.size());
        }
    );

    if (!delayed_)
    {
        if (input_data_size < samples_delay_)
        {
            // Do not start consuming the samples until we've created enough delay
            // to tolerate the scheduling imperfections.
            return;
        }
        else
        {
            delayed_ = true;
            int error = 0;
            if ((error = snd_pcm_prepare(pcm_device_)) < 0)
            {
                THROW(AlsaError(error)) << "Cannot prepare";
            }
        }
    }

    std::size_t written_frames = 0, remaining_frames = std::min(input_data_size, availableFrames(pcm_device_));

    while (remaining_frames > 0)
    {
        const T* buffers[ChannelsCount];
        forEachTupleElement(
            input,
            [=, &buffers](const auto& channel, auto channel_index)
            {
                buffers[channel_index] = &channel[written_frames];
            }
        );

        snd_pcm_sframes_t frames =
            snd_pcm_writen(
                pcm_device_,
                const_cast<void**>(reinterpret_cast<const void**>(buffers)),
                remaining_frames
            );

        if (frames == -EPIPE)
        {
            recoverFromUnderrun(pcm_device_);
        }
        else if (frames < 0)
        {
            THROW(AlsaError(int(frames))) << "Alsa error when writing frames";
        }
        else
        {
            remaining_frames -= std::size_t(frames);
            written_frames += std::size_t(frames);
        }
    }

    forEachTupleElement(
        input,
        [=](const auto& channel, auto /* channel_index */)
        {
            channel.advance(written_frames);
        }
    );
}

template class hvylya::filters::AlsaSink<float, 1>;
template class hvylya::filters::AlsaSink<float, 2>;
