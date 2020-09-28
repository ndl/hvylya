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
#include <hvylya/core/callable_traits.h>

namespace hvylya {
namespace filters {

// Adds const lvalue reference to the type.
template <typename T>
struct AddConstRef
{
    typedef const T& Type;
};

// Adds lvalue reference to the type.
template <typename T>
struct AddRef
{
    typedef T& Type;
};

// Provides access to mapper that wraps given type with
// given template class.
template <template <typename T> class Wrapper>
struct WrapWith
{
    template <typename T>
    struct Mapper
    {
        typedef Wrapper<T> Type;
    };
};

template <typename T>
struct CheckIsPodMapper
{
    static_assert(std::is_standard_layout<T>::value, "Only standard layout types are supported");
    typedef T Type;
};

// Converts type list with channels to the appropriate input tuple type.
template <typename TL>
struct InputsType
{
    typedef typename core::TypeListToTuple<
        typename core::TypeMapper<
            AddConstRef,
            typename core::TypeMapper<
                WrapWith<core::Slice>::template Mapper,
                typename core::TypeMapper<
                    CheckIsPodMapper,
                    TL
                >::Type
            >::Type
        >::Type
    >::Type Type;
};

// Converts type list with channels to the appropriate output tuple type.
template <typename TL>
struct OutputsType
{
    typedef typename core::TypeListToTuple<
        typename core::TypeMapper<
            AddRef,
            typename core::TypeMapper<
                WrapWith<core::Slice>::template Mapper,
                typename core::TypeMapper<
                    CheckIsPodMapper,
                    TL
                >::Type
            >::Type
        >::Type
    >::Type Type;
};

template <typename T>
struct TypeSizeMapper
{
    enum: std::size_t { Value = sizeof(T) };
};

// Utility struct to extract the type of the base class having
// the name of the derived class, needed to avoid repeating input / output
// template parameters in derived filters classes.
template <typename T>
struct FilterBaseType
{
    // Fake declaration for the subsequent typedef.
    template<class Base, class Derived>
    static Base filterBaseClassOf(Derived Base::*);

    typedef decltype(filterBaseClassOf(&T::classTypeMarker)) Type;
};

template <typename Callable, typename InputTypesList, typename OutputTypesList>
struct CallableArgsExtractorImpl { };

template <typename InputTypesList, typename OutputTypesList>
struct CallableArgsExtractorImpl<core::TypeList<>, InputTypesList, OutputTypesList>
{
    typedef InputTypesList InputTypes;
    typedef OutputTypesList OutputTypes;
};

template <typename InputTypesList, typename OutputTypesList, typename HeadType, typename... TailTypes>
struct CallableArgsExtractorImpl<core::TypeList<const HeadType&, TailTypes...>, InputTypesList, OutputTypesList>
{
    typedef CallableArgsExtractorImpl<
        core::TypeList<TailTypes...>,
        typename core::TypeListAppender<InputTypesList, HeadType>::Type,
        OutputTypesList
    > TailCallableArgsExtractorImpl;

    typedef typename TailCallableArgsExtractorImpl::InputTypes InputTypes;
    typedef typename TailCallableArgsExtractorImpl::OutputTypes OutputTypes;
};

template <typename InputTypesList, typename OutputTypesList, typename HeadType, typename... TailTypes>
struct CallableArgsExtractorImpl<core::TypeList<HeadType&, TailTypes...>, InputTypesList, OutputTypesList>
{
    typedef CallableArgsExtractorImpl<
        core::TypeList<TailTypes...>,
        InputTypesList,
        typename core::TypeListAppender<OutputTypesList, HeadType>::Type
    > TailCallableArgsExtractorImpl;

    typedef typename TailCallableArgsExtractorImpl::InputTypes InputTypes;
    typedef typename TailCallableArgsExtractorImpl::OutputTypes OutputTypes;
};

template <typename Callable>
struct CallableArgsExtractor
{
    typedef CallableArgsExtractorImpl<typename core::CallableTraits<Callable>::ArgsTypes, core::TypeList<>, core::TypeList<>> ImplType;

    typedef typename ImplType::InputTypes InputTypes;
    typedef typename ImplType::OutputTypes OutputTypes;
};

} // namespace filters
} // namespace hvylya
