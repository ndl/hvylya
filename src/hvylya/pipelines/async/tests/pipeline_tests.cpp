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

#include <hvylya/filters/connect.h>
#include <hvylya/filters/null_sink.h>
#include <hvylya/filters/mapper_filter.h>

#include <hvylya/pipelines/async/pipeline.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::filters;
using namespace hvylya::pipelines::async;

namespace {

const char* TestExceptionString = "test exception";

[[ noreturn ]] void func(float&)
{
    errno = EIO;
    THROW(IoError()) << TestExceptionString;
}

}

TEST(Pipeline, ExceptionPropagation)
{
    MapperFilter<decltype(&func)> source(&func);
    NullSink<float> sink;
    connect(source, sink);

    Pipeline pipeline;
    pipeline.add(source);

    bool caught = false;
    try
    {
        pipeline.run();
    }
    catch (const IoError& ex)
    {
        caught = true;
        EXPECT_EQ(0, strncmp(TestExceptionString, ex.what(), strlen(TestExceptionString)));
        EXPECT_NE(nullptr, strstr(ex.what(), "Input/output error"));
    }

    EXPECT_EQ(true, caught);
}
