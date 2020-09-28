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

#include <hvylya/core/fastsig.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::core;

namespace {

int multiply(int val0, int val1)
{
    return val0 * val1;
}

template <typename T>
class ResultsCombinerSum
{
  public:
    ResultsCombinerSum():
        value_(T())
    {
    }

    void combine(T&& value)
    {
        value_ += value;
    }

    T result()
    {
        return value_;
    }

  private:
    T value_;
};

}

TEST(FastSigTests, GlobalFun)
{
    Signal<int (int, int)> sig;
    sig.connect(&multiply);
    EXPECT_EQ(4, sig(2, 2));
}

TEST(FastSigTests, Lambdas)
{
    Signal<int (int, int)> sig;
    sig.connect([](int val0, int val1) { return val0 * val1; });
    EXPECT_EQ(4, sig(2, 2));
}

TEST(FastSigTests, VoidResult)
{
    Signal<void (int)> sig;
    int stored_val = 1;
    sig.connect([&stored_val](int val) { stored_val *= val; });
    sig(2);
    EXPECT_EQ(2, stored_val);
}

TEST(FastSigTests, VoidBoth)
{
    Signal<void ()> sig;
    int stored_val = 1;
    sig.connect([&stored_val]() { stored_val = 2; });
    sig();
    EXPECT_EQ(2, stored_val);
}

TEST(FastSigTests, EmptyCall)
{
    Signal<int ()> sig;
}

TEST(FastSigTests, MultipleSlots)
{
    Signal<void (int)> sig;
    int stored_val = 1;
    sig.connect([&stored_val](int val) { stored_val *= val; });
    sig.connect([&stored_val](int val) { stored_val *= val; });
    sig(2);
    EXPECT_EQ(4, stored_val);
}

TEST(FastSigTests, Disconnect)
{
    Signal<void (int)> sig;
    int stored_val = 1;
    auto conn = sig.connect([&stored_val](int val) { stored_val *= val; });
    sig.disconnect(conn);
    sig(2);
    EXPECT_EQ(1, stored_val);
}

TEST(FastSigTests, Combiner)
{
    Signal<int (int, int), ResultsCombinerSum<int>> sig;
    sig.connect([](int val0, int val1) { return val0 * val1; });
    sig.connect([](int val0, int val1) { return val0 * val1; });
    EXPECT_EQ(8, sig(2, 2));
}
