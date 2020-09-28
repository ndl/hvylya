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

#define PRAGMA_STRINGIFIED(P) _Pragma(#P)

#define DISABLE_WARNING(W) \
    PRAGMA_STRINGIFIED(clang diagnostic ignored W)

#define ENABLE_WARNING(W) \
    PRAGMA_STRINGIFIED(clang diagnostic warning W)

#define DISABLE_WARNING_PUSH(W) \
    PRAGMA_STRINGIFIED(clang diagnostic push) \
    DISABLE_WARNING(W)

#define DISABLE_WARNING_POP() \
    PRAGMA_STRINGIFIED(clang diagnostic pop)

#define ENABLE_WARNING_PUSH(W) \
    PRAGMA_STRINGIFIED(clang diagnostic push) \
    ENABLE_WARNING(W)

#define ENABLE_WARNING_POP() \
    PRAGMA_STRINGIFIED(clang diagnostic pop)
