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

#include <hvylya/core/exceptions.h>

using namespace hvylya::core;

ExceptionBase::ExceptionBase():
    exception_operator_(nullptr)
{
}

ExceptionBase::ExceptionBase(const ExceptionBase& ex):
    buffer_(ex.buffer_),
    message_(ex.message_),
    location_(ex.location_),
    details_(ex.details_),
    exception_operator_(ex.exception_operator_)
{
}

ExceptionBase::ExceptionBase(ExceptionBase&& ex):
    buffer_(std::move(ex.buffer_)),
    message_(std::move(ex.message_)),
    location_(std::move(ex.location_)),
    details_(std::move(ex.details_)),
    exception_operator_(ex.exception_operator_)
{
}

const char* ExceptionBase::what() const noexcept
{
    return message_.c_str();
}

std::unique_ptr<ExceptionBase> ExceptionBase::clone() const
{
    CHECK(exception_operator_);
    return exception_operator_(*this, ExceptionOperation::Clone);
}

void ExceptionBase::rethrow() const
{
    CHECK(exception_operator_);
    exception_operator_(*this, ExceptionOperation::Rethrow);
}

void ExceptionBase::setDetails(const char* details)
{
    details_ = details;
    update();
}

void ExceptionBase::update()
{
    message_ = buffer_;
    if (!location_.empty())
    {
        message_.append(" at ");
        message_.append(location_);
    }

    if (!details_.empty())
    {
        message_.append(": ");
        message_.append(details_);
    }
}

SystemError::SystemError()
{
    char buffer[1024] = { 0 };
    //FIXME: GNU version of strerror_r doesn't seem to use the buffer.
    setDetails(strerror_r(errno, buffer, sizeof(buffer)));
}
