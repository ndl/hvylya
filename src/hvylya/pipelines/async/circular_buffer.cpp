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

#include <hvylya/pipelines/async/circular_buffer.h>

#include <hvylya/filters/io_states.h>

#include <hvylya/pipelines/async/block.h>
#include <hvylya/pipelines/async/pipeline.h>

#include <sstream>

using namespace hvylya::core;
using namespace hvylya::pipelines::async;

CircularBufferReader::CircularBufferReader(Pipeline& pipeline, Block& block, std::size_t input_channel):
    output_(nullptr),
    pipeline_(pipeline),
    block_(block),
    input_channel_(input_channel),
    history_size_(block.inputState(input_channel).historySize()),
    delay_(block.inputState(input_channel).delay()),
    min_combined_input_size_(history_size_ + block.inputState(input_channel).requiredSize()),
    suggested_input_size_(history_size_ + block.inputState(input_channel).suggestedSize()),
    padding_(block.inputState(input_channel).padding()),
    input_index_(0),
    wrap_flag_(false)
{
    available_size_.store(0);
    wrapping_.store(false);
}

CircularBufferReader::CircularBufferReader(CircularBufferReader&& reader):
    output_(reader.output_),
    pipeline_(reader.pipeline_),
    block_(reader.block_),
    input_channel_(reader.input_channel_),
    history_size_(reader.history_size_),
    delay_(reader.delay_),
    min_combined_input_size_(reader.min_combined_input_size_),
    suggested_input_size_(reader.suggested_input_size_),
    padding_(reader.padding_),
    input_index_(reader.input_index_),
    wrap_flag_(reader.wrap_flag_)
{
    available_size_.store(reader.available_size_.load());
    wrapping_.store(reader.wrapping_.load());
}

void CircularBufferReader::reset()
{
    input_index_ = 0;
    wrap_flag_ = false;
    available_size_.store(0);
    wrapping_.store(false);
}

void CircularBufferReader::setWriter(CircularBufferWriter& writer)
{
    output_ = &writer;
}

void CircularBufferReader::adjust()
{
    if (wrap_flag_ != output_->wrap_flag_)
    {
        // Output has wrapped already. Are we close enough to the end of the buffer
        // to wrap our input as well?
        if (output_->current_size_ - input_index_ <= output_->overlap_)
        {
            input_index_ = output_->overlap_ - (output_->current_size_ - input_index_);
            wrap_flag_ = !wrap_flag_;
        }
    }

    available_size_.store(
        (wrap_flag_ == output_->wrap_flag_) ?
        output_->output_index_ - input_index_ :
        output_->current_size_ - input_index_
    );

    wrapping_.store(
        (wrap_flag_ == output_->wrap_flag_) ?
        output_->data_size_ - input_index_ < suggested_input_size_ :
        output_->current_size_ - input_index_ < suggested_input_size_
    );

    // Notify pipeline about input size change.
    pipeline_.inputAvailableSizeChanged(block_, input_channel_);
}

void CircularBufferReader::advance(std::size_t size)
{
    std::lock_guard<std::mutex> lock(output_->mutex_);

    output_->verify();
    CHECK_LE(input_index_ + size, output_->data_size_) << "advance(" << size << "): "<< output_->internalState();
    CHECK(wrap_flag_ != output_->wrap_flag_ || input_index_ + size <= output_->output_index_) << "advance(" << size << "): "<< output_->internalState();

    input_index_ += size;
    adjust();

    // Finished our internal data update - notify output about them.
    output_->updateMinInputIndex();

    output_->verify();
}

std::size_t CircularBufferReader::availableSize() const
{
    return available_size_.load();
}

bool CircularBufferReader::wrapping() const
{
    return wrapping_.load();
}

UntypedSlice CircularBufferReader::slice()
{
    std::lock_guard<std::mutex> lock(output_->mutex_);
    std::size_t available_size = available_size_.load();

    output_->verify();
    CHECK_GE(available_size, min_combined_input_size_) << output_->internalState();
    CHECK_LE(input_index_ + available_size, output_->data_size_) << output_->internalState();
    CHECK(wrap_flag_ != output_->wrap_flag_ || input_index_ + available_size <= output_->output_index_) << output_->internalState();

    return core::UntypedSlice(&output_->data_[input_index_ * output_->type_size_], available_size);
}

CircularBufferWriter::CircularBufferWriter(Pipeline& pipeline, Block& block, std::size_t output_channel, std::size_t buffer_size):
    pipeline_(pipeline),
    block_(block),
    output_channel_(output_channel),
    min_output_size_(block.outputState(output_channel).requiredSize()),
    suggested_output_size_(std::max(min_output_size_, block.outputState(output_channel).suggestedSize())),
    padding_(block.outputState(output_channel).padding()),
    data_size_(buffer_size - padding_),
    buffer_size_(buffer_size),
    overlap_(0),
    current_size_(data_size_),
    output_index_(0),
    min_input_index_(data_size_),
    type_size_(block.outputState(output_channel).typeSize()),
    alignment_(type_size_ < MaxSimdByteSize && !(MaxSimdByteSize % type_size_) ? MaxSimdByteSize / type_size_ : 1),
    wrap_flag_(false),
    min_input_wrap_flag_(false),
    data_(buffer_size * type_size_)
{
    CHECK_LE(min_output_size_, data_size_);
    CHECK_EQ(0, data_size_ % alignment_);
    available_size_.store(data_size_);
    wrapping_.store(false);
}

CircularBufferWriter::CircularBufferWriter(CircularBufferWriter&& writer):
    readers_(std::move(writer.readers_)),
    pipeline_(writer.pipeline_),
    block_(writer.block_),
    output_channel_(writer.output_channel_),
    min_output_size_(writer.min_output_size_),
    suggested_output_size_(writer.suggested_output_size_),
    padding_(writer.padding_),
    data_size_(writer.data_size_),
    buffer_size_(writer.buffer_size_),
    overlap_(writer.overlap_),
    current_size_(writer.current_size_),
    output_index_(writer.output_index_),
    min_input_index_(writer.min_input_index_),
    type_size_(writer.type_size_),
    alignment_(writer.alignment_),
    wrap_flag_(writer.wrap_flag_),
    min_input_wrap_flag_(writer.min_input_wrap_flag_),
    data_(std::move(writer.data_))
{
    available_size_.store(writer.available_size_.load());
    wrapping_.store(writer.wrapping_.load());
}

void CircularBufferWriter::reset()
{
    current_size_ = data_size_;
    wrap_flag_ = false;
    min_input_wrap_flag_ = false;
    wrapping_.store(false);

    std::size_t max_history_size = 0;
    for (auto input: readers_)
    {
        max_history_size = std::max<std::size_t>(max_history_size, input->history_size_);
    }

    max_history_size = roundUp(max_history_size, alignment_);

    output_index_ = max_history_size;
    min_input_index_ = max_history_size;
    available_size_.store(data_size_ - output_index_);

    for (auto input: readers_)
    {
        input->input_index_ = max_history_size + input->delay_ - input->history_size_;
        min_input_index_ = std::min(min_input_index_, input->input_index_);
    }

    CHECK_LT(min_input_index_, data_size_);
}

void CircularBufferWriter::addReader(CircularBufferReader& reader)
{
    readers_.push_back(&reader);

    padding_ = std::max(padding_, reader.padding_);
    data_size_ = buffer_size_ - padding_;
    current_size_ = data_size_;
    CHECK_EQ(0, data_size_ % alignment_);

    overlap_ = roundUp(std::max(overlap_, reader.min_combined_input_size_), alignment_);
    CHECK_LE(2 * overlap_ + min_output_size_, data_size_);

    reset();
}

void CircularBufferWriter::verify() const
{
    CHECK_LE(output_index_, data_size_) << internalState();
    CHECK_LE(current_size_, data_size_) << internalState();
    CHECK_LE(min_input_index_, current_size_) << internalState();

    for (auto input: readers_)
    {
        CHECK(wrap_flag_ == input->wrap_flag_ || input->input_index_ >= min_input_index_) << internalState();
        CHECK_LE(input->input_index_ + input->history_size_, data_size_) << internalState();
        CHECK(wrap_flag_ == input->wrap_flag_ || input->input_index_ + input->history_size_ <= current_size_) << internalState();
        CHECK(wrap_flag_ != min_input_wrap_flag_ || output_index_ >= input->input_index_) << internalState();
    }
}

std::string CircularBufferWriter::internalState() const
{
    std::stringstream os;
    os << "output_index_ = " << output_index_ <<
        ", current_size_ = " << current_size_ <<
        ", overlap_ = " << overlap_ <<
        ", output_channel_ = " << output_channel_ <<
        ", min_output_size_ = " << min_output_size_ <<
        ", available_size_ = " << available_size_.load() <<
        ", min_input_index_ = " << min_input_index_ <<
        ", wrap_flag_ = " << wrap_flag_ << std::endl;

    for (auto input: readers_)
    {
        os << "input_channel_ = " << input->input_channel_ <<
            ", history_size = " << input->history_size_ <<
            ", delay = " << input->delay_ <<
            ", min_combined_input_size_ = " << input->min_combined_input_size_ <<
            ", input_index_ = " << input->input_index_ <<
            ", available_size_ = " << input->available_size_.load() <<
            ", wrap_flag_ = " << input->wrap_flag_ << std::endl;
    }

    return os.str();
}

void CircularBufferWriter::adjust()
{
    if (wrap_flag_ == min_input_wrap_flag_ && output_index_ + min_output_size_ > data_size_)
    {
        if (min_input_index_ >= overlap_)
        {
            current_size_ = output_index_;
            std::copy(&data_[(output_index_ - overlap_) * type_size_], &data_[output_index_ * type_size_], &data_[0]);
            output_index_ = overlap_;
            wrap_flag_ = !wrap_flag_;
        }
    }

    available_size_.store(
        (wrap_flag_ == min_input_wrap_flag_) ?
        data_size_ - output_index_ :
        min_input_index_ - output_index_
    );

    wrapping_.store(data_size_ - output_index_ < suggested_output_size_);

    // Notify pipeline about output size change.
    pipeline_.outputAvailableSizeChanged(block_, output_channel_);
}

void CircularBufferWriter::advance(std::size_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);

    verify();
    CHECK_LE(output_index_ + size, data_size_) << internalState();
    CHECK(wrap_flag_ == min_input_wrap_flag_ || output_index_ + size <= min_input_index_) << internalState();

    output_index_ += size;
    adjust();

    // Finished our internal data update - notify inputs about them.
    for (auto input: readers_)
    {
        input->adjust();
    }

    // min_input_index_ might have changed after input->adjust() calls.
    updateMinInputIndex();

    verify();
}

void CircularBufferWriter::updateMinInputIndex()
{
    std::size_t new_min_input_index_above = std::numeric_limits<std::size_t>::max();
    std::size_t new_min_input_index_below = std::numeric_limits<std::size_t>::max();

    for (auto input: readers_)
    {
        std::size_t input_index = input->input_index_;

        if (wrap_flag_ != input->wrap_flag_ && input_index < new_min_input_index_above)
        {
            new_min_input_index_above = input_index;
        }

        if (wrap_flag_ == input->wrap_flag_ && input_index < new_min_input_index_below)
        {
            new_min_input_index_below = input_index;
        }
    }

    std::size_t prev_min_input_index = min_input_index_;
    if (new_min_input_index_above != std::numeric_limits<std::size_t>::max())
    {
        min_input_index_ = new_min_input_index_above;
        min_input_wrap_flag_ = !wrap_flag_;
    }
    else
    {
        CHECK_NE(new_min_input_index_below, std::numeric_limits<std::size_t>::max()) << internalState();
        min_input_index_ = new_min_input_index_below;
        min_input_wrap_flag_ = wrap_flag_;
    }

    // Notify the output about the change, if there was any.
    if (prev_min_input_index != min_input_index_)
    {
        adjust();
    }
}

std::size_t CircularBufferWriter::availableSize() const
{
    return available_size_.load();
}

bool CircularBufferWriter::wrapping() const
{
    return wrapping_.load();
}

UntypedSlice CircularBufferWriter::slice()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t available_size = availableSize();

    verify();
    CHECK_GE(available_size, min_output_size_) << internalState();
    CHECK_LE(output_index_ + available_size, data_size_) << internalState();
    CHECK(wrap_flag_ == min_input_wrap_flag_ || output_index_ + available_size <= min_input_index_) << internalState();

    return core::UntypedSlice(&data_[output_index_ * type_size_], available_size);
}
