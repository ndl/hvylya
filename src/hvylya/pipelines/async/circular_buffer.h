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

#include <hvylya/core/aligned_vector.h>
#include <hvylya/core/untyped_slice.h>

#include <mutex>

namespace hvylya {
namespace pipelines {
namespace async {

class CircularBufferWriter;
class Pipeline;
class Block;

class CircularBufferReader: core::NonCopyable
{
  public:
    CircularBufferReader(Pipeline& pipeline, Block& block, std::size_t input_channel);

    CircularBufferReader(CircularBufferReader&& reader);

    std::size_t availableSize() const;

    bool wrapping() const;

    void advance(std::size_t size);

    core::UntypedSlice slice();

    void setWriter(CircularBufferWriter& writer);

    void reset();

 private:
    friend class CircularBufferWriter;

    CircularBufferWriter* output_;
    Pipeline& pipeline_;
    Block& block_;
    std::size_t input_channel_, history_size_, delay_, min_combined_input_size_, suggested_input_size_, padding_;
    std::size_t input_index_;
    std::atomic<std::size_t> available_size_;
    std::atomic<bool> wrapping_;
    bool wrap_flag_;

    void adjust();
};

class CircularBufferWriter: core::NonCopyable
{
  public:
    CircularBufferWriter(Pipeline& pipeline, Block& block, std::size_t output_channel, std::size_t buffer_size);

    CircularBufferWriter(CircularBufferWriter&& writer);

    std::size_t availableSize() const;

    bool wrapping() const;

    void advance(std::size_t offset);

    core::UntypedSlice slice();

    void addReader(CircularBufferReader& reader);

    std::string internalState() const;

    void reset();

 private:
    friend class CircularBufferReader;

    std::vector<CircularBufferReader*> readers_;
    Pipeline& pipeline_;
    Block& block_;
    std::size_t output_channel_, min_output_size_, suggested_output_size_, padding_, data_size_, buffer_size_, overlap_;
    std::size_t current_size_, output_index_, min_input_index_, type_size_, alignment_;
    std::atomic<std::size_t> available_size_;
    std::atomic<bool> wrapping_;
    bool wrap_flag_, min_input_wrap_flag_;
    core::AlignedVector<std::int8_t> data_;
    mutable std::mutex mutex_;

    void verify() const;

    void updateMinInputIndex();

    void adjust();
};

} // namespace async
} // namespace pipelines
} // namespace hvylya
