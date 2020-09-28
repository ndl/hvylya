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

#include <hvylya/core/common.h>

namespace hvylya {
namespace filters {

class InputState
{
  public:
    InputState():
        history_size_(0),
        delay_(0),
        required_size_(1),
        suggested_size_(0),
        padding_(0),
        may_consume_nothing_(false)
    {
    }

    std::size_t typeSize() const
    {
        return type_size_;
    }

    std::size_t historySize() const
    {
        return history_size_;
    }

    std::size_t delay() const
    {
        return delay_;
    }

    std::size_t requiredSize() const
    {
        return required_size_;
    }

    std::size_t suggestedSize() const
    {
        return suggested_size_;
    }

    std::size_t padding() const
    {
        return padding_;
    }

    bool mayConsumeNothing() const
    {
        return may_consume_nothing_;
    }

    void setTypeSize(std::size_t type_size)
    {
        type_size_ = type_size;
    }

    void setHistorySize(std::size_t history_size)
    {
        history_size_ = history_size;
    }

    void setDelay(std::size_t delay)
    {
        delay_ = delay;
    }

    void setRequiredSize(std::size_t required_size)
    {
        required_size_ = required_size;
        suggested_size_ = std::max(suggested_size_, required_size);
    }

    void setSuggestedSize(std::size_t suggested_size)
    {
        CHECK_GE(suggested_size, required_size_);
        suggested_size_ = suggested_size;
    }

    void setPadding(std::size_t padding)
    {
        padding_ = padding;
    }

    void setMayConsumeNothing(bool may_consume_nothing)
    {
        may_consume_nothing_ = may_consume_nothing;
    }

  private:
    std::size_t type_size_, history_size_, delay_, required_size_, suggested_size_, padding_;
    bool may_consume_nothing_;
};

class OutputState
{
  public:
    OutputState():
        required_size_(1),
        provided_size_(1),
        suggested_size_(0),
        padding_(0)
    {
        setEof(false);
    }

    std::size_t typeSize() const
    {
        return type_size_;
    }

    std::size_t requiredSize() const
    {
        return required_size_;
    }

    std::size_t providedSize() const
    {
        return provided_size_;
    }

    std::size_t suggestedSize() const
    {
        return suggested_size_;
    }

    std::size_t padding() const
    {
        return padding_;
    }

    bool eof() const
    {
        return eof_.load();
    }

    void setTypeSize(std::size_t type_size)
    {
        type_size_ = type_size;
    }

    void setRequiredSize(std::size_t required_size)
    {
        required_size_ = required_size;
        suggested_size_ = std::max(suggested_size_, required_size);
    }

    void setProvidedSize(std::size_t provided_size)
    {
        provided_size_ = provided_size;
    }

    void setSuggestedSize(std::size_t suggested_size)
    {
        CHECK_GE(suggested_size, required_size_);
        suggested_size_ = suggested_size;
    }

    void setPadding(std::size_t padding)
    {
        padding_ = padding;
    }

    void setEof(bool eof)
    {
        eof_.store(eof);
    }

  private:
    std::size_t type_size_, required_size_, provided_size_, suggested_size_, padding_;
    // Unlike all other values, this one can change while pipeline is running, so make sure
    // we're accessing the consistent version of it.
    std::atomic<bool> eof_;
};

} // namespace filters
} // namespace hvylya
