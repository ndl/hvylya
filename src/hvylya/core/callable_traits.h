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

#include <hvylya/core/type_list.h>

namespace hvylya {
namespace core {

// Generic callable objects, including lambda functions.
template <typename Class>
struct CallableTraits: public CallableTraits<decltype(&Class::operator ())> { };

template <typename Result, typename... Args>
struct CallableTraits<Result(Args...)>
{
    typedef Result ResultType;
    typedef TypeList<Args...> ArgsTypes;
};

// Normal functions pointers.
template <typename Result, typename... Args>
struct CallableTraits<Result (*)(Args...)>: public CallableTraits<Result (Args...)> { };

// Member functions pointers.
template <typename Class, typename Result, typename... Args>
struct CallableTraits<Result (Class::*)(Args...)>: public CallableTraits<Result (Args...)> { };

// Const member functions pointers.
template <typename Class, typename Result, typename... Args>
struct CallableTraits<Result (Class::*)(Args...) const>: public CallableTraits<Result (Args...)> { };

} // namespace core
} // namespace hvylya
