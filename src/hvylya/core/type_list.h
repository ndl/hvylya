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
namespace core {

// Type list structure.
template <typename... Types>
struct TypeList
{
    enum { Size = sizeof...(Types) };
};

// Type mapper to apply type operations to type lists.
template <template <typename T> class Mapper, typename... Types>
struct TypeMapper { };

template <template <typename T> class Mapper, typename... Types>
struct TypeMapper<Mapper, TypeList<Types...>>
{
    typedef TypeList<typename Mapper<Types>::Type...> Type;
};

// Convert type list to tuple.
template <typename... Types>
struct TypeListToTuple { };

template <typename... Types>
struct TypeListToTuple<TypeList<Types...>>
{
    typedef std::tuple<Types...> Type;
};

// Convert tuple to type list.
template <typename... Types>
struct TupleToTypeList { };

template <typename... Types>
struct TupleToTypeList<std::tuple<Types...>>
{
    typedef TypeList<Types...> Type;
};

// Extract type at given index.
template <typename TL, std::size_t Index>
struct TypeAt { };

template <typename T, typename... Types>
struct TypeAt<TypeList<T, Types...>, 0>
{
    typedef T Type;
};

template <std::size_t Index, typename T, typename... Types>
struct TypeAt<TypeList<T, Types...>, Index>
{
    typedef typename TypeAt<TypeList<Types...>, Index - 1>::Type Type;
};

// Append the type to the end of the list.
template <typename TL, typename ExtraType>
struct TypeListAppender { };

template <typename ExtraType, typename... Types>
struct TypeListAppender<TypeList<Types...>, ExtraType>
{
    typedef TypeList<Types..., ExtraType> Type;
};

// Create TypeList of Count instances of type T.
template <typename T, std::size_t Count>
struct TypeDuplicator
{
    typedef typename TypeListAppender<
        typename TypeDuplicator<T, Count - 1>::Type,
        T
    >::Type Type;
};

template <typename T>
struct TypeDuplicator<T, 0>
{
    typedef TypeList<> Type;
};

// Dummy function to terminate recursion.
template <
    typename Tuple,
    typename Callable,
    std::size_t TupleElementIndex = 0,
    typename std::enable_if<
        TupleElementIndex == std::tuple_size<Tuple>::value
    >::type* = nullptr
>
void forEachTupleElement(const Tuple& /* tuple */, Callable /* callable */) { }

template <
    typename Tuple,
    typename Callable,
    std::size_t TupleElementIndex = 0,
    typename std::enable_if<
        TupleElementIndex < std::tuple_size<Tuple>::value
    >::type* = nullptr
>
void forEachTupleElement(const Tuple& tuple, Callable callable)
{
    callable(std::get<TupleElementIndex>(tuple), TupleElementIndex);
    forEachTupleElement<Tuple, Callable, TupleElementIndex + 1>(tuple, callable);
}

template <
    typename Tuple,
    typename Callable,
    std::size_t TupleElementIndex = 0,
    typename std::enable_if<
        TupleElementIndex < std::tuple_size<Tuple>::value
    >::type* = nullptr
>
void forEachTupleElement(Tuple& tuple, Callable callable)
{
    callable(std::get<TupleElementIndex>(tuple), TupleElementIndex);
    forEachTupleElement<Tuple, Callable, TupleElementIndex + 1>(tuple, callable);
}

// Dummy function to terminate recursion.
template <
    typename TL,
    template <typename T> class TypeToValueMapper,
    std::size_t TypeElementIndex = 0,
    typename Callable,
    typename std::enable_if<
        TypeElementIndex == TL::Size
    >::type* = nullptr
>
void forEachType(Callable /* callable */) { }

template <
    typename TL,
    template <typename T> class TypeToValueMapper,
    std::size_t TypeElementIndex = 0,
    typename Callable,
    typename std::enable_if<
        TypeElementIndex < TL::Size
    >::type* = nullptr
>
void forEachType(Callable callable)
{
    callable(TypeToValueMapper<typename TypeAt<TL, TypeElementIndex>::Type>::Value, TypeElementIndex);
    forEachType<TL, TypeToValueMapper, TypeElementIndex + 1>(callable);
}

} // namespace common
} // namespace hvylya
