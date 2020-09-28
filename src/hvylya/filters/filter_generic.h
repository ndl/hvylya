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

#include <hvylya/core/slice.h>
#include <hvylya/filters/ifilter.h>
#include <hvylya/filters/io_states.h>
#include <hvylya/filters/type_utils.h>

namespace hvylya {
namespace filters {

template <typename InputChannelsParam, typename OutputChannelsParam>
class FilterGeneric: public IFilter
{
  public:
    typedef InputChannelsParam InputChannels;
    typedef OutputChannelsParam OutputChannels;

    typedef typename InputsType<InputChannels>::Type Inputs;
    typedef typename OutputsType<OutputChannels>::Type Outputs;

    enum: std::size_t
    {
        InputsCount = InputChannels::Size,
        OutputsCount = OutputChannels::Size,
        DefaultSuggestedSize = 4096
    };

    FilterGeneric()
    {
        core::forEachType<InputChannels, TypeSizeMapper>(
            [this](std::size_t type_size, std::size_t input_channel)
            {
                input_states_[input_channel].setTypeSize(type_size);
                input_states_[input_channel].setSuggestedSize(DefaultSuggestedSize);
            }
        );
        core::forEachType<OutputChannels, TypeSizeMapper>(
            [this](std::size_t type_size, std::size_t output_channel)
            {
                output_states_[output_channel].setTypeSize(type_size);
                output_states_[output_channel].setSuggestedSize(DefaultSuggestedSize);
            }
        );
    }

    virtual std::size_t inputChannelsCount() const override
    {
        return InputsCount;
    }

    virtual std::size_t outputChannelsCount() const override
    {
        return OutputsCount;
    }

    virtual const InputState& inputState(std::size_t input_channel) const override
    {
        CHECK_LT(input_channel, InputsCount);
        return input_states_[input_channel];
    }

    virtual const OutputState& outputState(std::size_t output_channel) const override
    {
        CHECK_LT(output_channel, OutputsCount);
        return output_states_[output_channel];
    }

    InputState& inputState(std::size_t input_channel)
    {
        CHECK_LT(input_channel, InputsCount);
        return input_states_[input_channel];
    }

    OutputState& outputState(std::size_t output_channel)
    {
        CHECK_LT(output_channel, OutputsCount);
        return output_states_[output_channel];
    }

    virtual const std::vector<Channel>& sources(std::size_t input_channel) const override
    {
        CHECK_LT(input_channel, InputsCount);
        return sources_[input_channel];
    }

    virtual const std::vector<Channel>& sinks(std::size_t output_channel) const override
    {
        CHECK_LT(output_channel, OutputsCount);
        return sinks_[output_channel];
    }

    virtual void process(core::UntypedSlice inputs[], core::UntypedSlice outputs[]) override
    {
        processImpl<0, 0>(inputs, outputs, std::make_tuple(), std::make_tuple());
    }

    virtual void reset() override
    {
        // Do nothing in the base version for now.
    }

    virtual void process(const Inputs& input, Outputs& output) = 0;

  protected:
    virtual void addSource(Channel source_output_channel, std::size_t input_channel) override
    {
        CHECK_LT(input_channel, InputsCount);
        sources_[input_channel].push_back(source_output_channel);
    }

    virtual void addSink(Channel sink_input_channel, std::size_t output_channel) override
    {
        CHECK_LT(output_channel, OutputsCount);
        sinks_[output_channel].push_back(sink_input_channel);
    }

  private:
    // Needed for &T::classTypeMarker to compile.
    template <typename T>
    friend struct FilterBaseType;

    // Needed for inputChannel / outputChannel.
    template <typename Filter0, std::size_t Index0, typename Filter1, std::size_t Index1>
    friend void connectImpl(ChannelGeneric<Filter0, Index0> channel0, ChannelGeneric<Filter1, Index1> channel1);

// It's OK in this case as we'll have zero-length inputs for sources and zero-length outputs for sinks.
DISABLE_WARNING_PUSH("-Wzero-length-array")
    InputState input_states_[InputsCount];
    OutputState output_states_[OutputsCount];
    std::vector<Channel> sources_[InputsCount], sinks_[OutputsCount];
DISABLE_WARNING_POP()

    Channel inputChannel(std::size_t input_index)
    {
        return Channel(*this, input_index);
    }

    Channel outputChannel(std::size_t output_index)
    {
        return Channel(*this, output_index);
    }

    template <
        std::size_t InputIndex,
        std::size_t OutputIndex,
        typename... InputTypes,
        typename... OutputTypes,
        typename std::enable_if<InputIndex < InputsCount>::type* = nullptr
    >
    void processImpl(
        core::UntypedSlice untyped_inputs[],
        core::UntypedSlice untyped_outputs[],
        std::tuple<InputTypes...> typed_inputs,
        std::tuple<OutputTypes...> typed_outputs
    )
    {
        typedef typename core::TypeAt<InputChannels, InputIndex>::Type InputType;

        auto& untyped_input = untyped_inputs[InputIndex];
        auto typed_input = core::Slice<InputType>(reinterpret_cast<InputType*>(untyped_input.data()), untyped_input.size());

        processImpl<InputIndex + 1, OutputIndex>(
            untyped_inputs,
            untyped_outputs,
            std::tuple_cat(typed_inputs, std::make_tuple(std::cref(typed_input))),
            typed_outputs
        );

        untyped_input.advance(typed_input.advancedSize());
    }

    template <
        std::size_t InputIndex,
        std::size_t OutputIndex,
        typename... InputTypes,
        typename... OutputTypes,
        typename std::enable_if<InputIndex == InputsCount>::type* = nullptr,
        typename std::enable_if<OutputIndex < OutputsCount>::type* = nullptr
    >
    void processImpl(
        core::UntypedSlice untyped_inputs[],
        core::UntypedSlice untyped_outputs[],
        std::tuple<InputTypes...> typed_inputs,
        std::tuple<OutputTypes...> typed_outputs
    )
    {
        typedef typename core::TypeAt<OutputChannels, OutputIndex>::Type OutputType;

        auto& untyped_output = untyped_outputs[OutputIndex];
        auto typed_output = core::Slice<OutputType>(reinterpret_cast<OutputType*>(untyped_output.data()), untyped_output.size());

        processImpl<InputIndex, OutputIndex + 1>(
            untyped_inputs,
            untyped_outputs,
            typed_inputs,
            std::tuple_cat(typed_outputs, std::make_tuple(std::ref(typed_output)))
        );

        untyped_output.advance(typed_output.advancedSize());
    }

    template <
        std::size_t InputIndex,
        std::size_t OutputIndex,
        typename... InputTypes,
        typename... OutputTypes,
        typename std::enable_if<InputIndex == InputsCount>::type* = nullptr,
        typename std::enable_if<OutputIndex == OutputsCount>::type* = nullptr
    >
    void processImpl(
        core::UntypedSlice /* untyped_inputs */[],
        core::UntypedSlice /* untyped_outputs */[],
        std::tuple<InputTypes...> typed_inputs,
        std::tuple<OutputTypes...> typed_outputs
    )
    {
        process(typed_inputs, typed_outputs);
    }

    // Needed for FilterBaseType.
    void classTypeMarker() { }
};

} // namespace filters
} // namespace hvylya
