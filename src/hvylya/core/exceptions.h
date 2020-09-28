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

#define THROW(ExObj) throw hvylya::core::augmentException(ExObj, __FILE__, __LINE__, __func__)

namespace hvylya {
namespace core {

class ExceptionBase;

enum class ExceptionOperation
{
    Clone,
    Rethrow
};

typedef std::unique_ptr<ExceptionBase> (*ExceptionOperator)(const ExceptionBase&, ExceptionOperation);

class ExceptionBase: public std::exception
{
  public:
    ExceptionBase();

    ExceptionBase(const ExceptionBase& ex);

    ExceptionBase(ExceptionBase&& ex);

    virtual const char* what() const noexcept override;

    std::unique_ptr<ExceptionBase> clone() const;

    void rethrow() const;

    void setDetails(const char* details);

  private:
    template <typename T>
    friend T& augmentException(T&, const char*, std::size_t, const char*);

    template <typename ExceptionType, typename ValueType>
    friend typename std::enable_if<std::is_base_of<ExceptionBase, ExceptionType>::value, ExceptionType&>::type
    operator << (ExceptionType&, const ValueType&);

    std::string buffer_, message_, location_, details_;
    ExceptionOperator exception_operator_;

    void update();
};

template <typename ExceptionType>
std::unique_ptr<ExceptionBase> exceptionOperator(const ExceptionBase& ex, ExceptionOperation op)
{
    if (op == ExceptionOperation::Clone)
    {
        return std::make_unique<ExceptionType>(static_cast<const ExceptionType&>(ex));
    }
    else
    {
        throw static_cast<const ExceptionType&>(ex);
    }
}

template <typename ExceptionType>
ExceptionType& augmentException(ExceptionType& ex, const char* file, std::size_t line, const char* func)
{
    ex.exception_operator_ = &exceptionOperator<ExceptionType>;
    ex.location_ = fmt::format("{0}:{1}:{2}", file, line, func);
    ex.update();
    return ex;
}

template <typename ExceptionType>
ExceptionType augmentException(ExceptionType&& ex, const char* file, std::size_t line, const char* func)
{
    ExceptionType new_ex(std::move(ex));
    augmentException(new_ex, file, line, func);
    return std::move(new_ex);
}

template <typename ExceptionType, typename ValueType>
typename std::enable_if<std::is_base_of<ExceptionBase, ExceptionType>::value, ExceptionType&>::type
operator << (ExceptionType& ex, const ValueType& value)
{
    ex.buffer_.append(fmt::format(value));
    ex.update();
    return ex;
}

template <typename ExceptionType, typename ValueType>
typename std::enable_if<std::is_base_of<ExceptionBase, ExceptionType>::value, ExceptionType>::type
operator << (ExceptionType&& ex, const ValueType& value)
{
    ExceptionType new_ex(std::move(ex));
    new_ex << value;
    return std::move(new_ex);
}

class SystemError: public ExceptionBase
{
  public:
    SystemError();

    SystemError(const SystemError& ex) = default;

    SystemError(SystemError&& ex) = default;
};

class IoError: public SystemError
{
  public:
    IoError() = default;

    IoError(const IoError& ex) = default;

    IoError(IoError&& ex) = default;
};

} // namespace core
} // namespace hvylya
