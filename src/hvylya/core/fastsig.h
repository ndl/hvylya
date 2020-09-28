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

#include <hvylya/core/callable_traits.h>

namespace hvylya {
namespace core {

// Note that `value_` will be initialized only if there's
// default constructor for `T`.
// In particular, all primitive types won't be initialized.
template <typename T>
class ResultsCombinerKeepLast
{
  public:
    void combine(T&& value)
    {
        value_ = value;
    }

    T result()
    {
        return value_;
    }

  private:
    T value_;
};

template <
    typename Signature,
    typename ResultsCombiner = ResultsCombinerKeepLast<typename hvylya::core::CallableTraits<Signature>::ResultType>
>
class Signal;

template <typename Signature>
class Connection
{
  private:
    template <typename SignatureParam, typename ResultsCombiner>
    friend class Signal;

    Connection(typename std::list<std::function<Signature>>::iterator it): it_(it) { }

    typename std::list<std::function<Signature>>::iterator it_;
};

template <typename Result, typename... Args, typename ResultsCombiner>
class Signal<Result (Args...), ResultsCombiner>
{
  public:
    typedef Result (Signature)(Args...);

    template <typename Func>
    Connection<Signature> connect(Func slot)
    {
        slots_.emplace_back(slot);
        return Connection<Signature>(--slots_.end());
    }

    void disconnect(Connection<Signature> connection)
    {
        slots_.erase(connection.it_);
    }

    template <typename ResultParam = Result>
    typename std::enable_if<std::is_void<ResultParam>::value, Result>::type 
    operator ()(Args... args) const
    {
        for (const auto& slot: slots_)
        {
            slot(std::forward<Args>(args)...);
        }
    }

    template <typename ResultParam = Result>
    typename std::enable_if<!std::is_void<ResultParam>::value, Result>::type 
    operator ()(Args... args) const
    {
        ResultsCombiner combiner;

        for (const auto& slot: slots_)
        {
            combiner.combine(slot(std::forward<Args>(args)...));
        }

        return combiner.result();
    }

  private:
    friend class Connection<Signature>;

    std::list<std::function<Signature>> slots_;
};

} // namespace common
} // namespace hvylya
