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

#include <hvylya/core/defines.h>

#include <algorithm>
#include <atomic>
#include <complex>
#include <cstddef>
#include <ctime>
#include <cmath>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <limits.h>
#include <stdlib.h>

#include <glog/logging.h>

//DISABLE_WARNING_PUSH("-Wundef")
//DISABLE_WARNING_PUSH("-Wdocumentation-unknown-command")
//DISABLE_WARNING_PUSH("-Wsign-conversion")
//DISABLE_WARNING_PUSH("-Wswitch-enum")
#include <fmt/format.h>
//DISABLE_WARNING_POP()
//DISABLE_WARNING_POP()
//DISABLE_WARNING_POP()
//DISABLE_WARNING_POP()

#include <hvylya/core/math_utils.h>
#include <hvylya/core/non_copyable.h>
#include <hvylya/core/type_list.h>
#include <hvylya/core/exceptions.h>
