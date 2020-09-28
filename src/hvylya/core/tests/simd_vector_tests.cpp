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

#include <hvylya/core/simd_vector.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::core;

namespace {

template <typename SimdByteSize>
class SimdVectorTest: public ::testing::Test
{
  public:
    typedef SimdVectorImpl<float, SimdByteSize::Value, Aligned> Vector;
    typedef SimdVectorImpl<std::complex<float>, SimdByteSize::Value, Aligned> ComplexVector;

    void testAddFloat()
    {
        Vector vec, vec2;
        fillVector(vec, 1.0f);
        fillVector(vec2, 2.0f);

        Vector vec3 = vec + vec2;

        for (std::size_t i = 0; i < Vector::Elements; ++i)
        {
            EXPECT_EQ(3.0f * i, vec3[i]);
        }

        vec3 += vec2;

        for (std::size_t i = 0; i < Vector::Elements; ++i)
        {
            EXPECT_EQ(5.0f * i, vec3[i]);
        }
    }

    void testSubtractFloat()
    {
        Vector vec, vec2;
        fillVector(vec, 1.0f);
        fillVector(vec2, 2.0f);

        Vector vec3 = vec - vec2;

        for (std::size_t i = 0; i < Vector::Elements; ++i)
        {
            EXPECT_EQ(-1.0f * i, vec3[i]);
        }

        vec3 -= vec2;

        for (std::size_t i = 0; i < Vector::Elements; ++i)
        {
            EXPECT_EQ(-3.0f * i, vec3[i]);
        }
    }

    void testMultiplyFloat()
    {
        Vector vec, vec2;
        fillVector(vec, 1.0f);
        fillVector(vec2, 2.0f);

        Vector vec3 = vec * vec2;

        for (std::size_t i = 0; i < Vector::Elements; ++i)
        {
            EXPECT_EQ(2.0f * i * i, vec3[i]);
        }

        vec3 *= vec2;

        for (std::size_t i = 0; i < Vector::Elements; ++i)
        {
            EXPECT_EQ(4.0f * i * i * i, vec3[i]);
        }
    }

    void testAddComplexFloat()
    {
        ComplexVector vec, vec2;
        fillVector(vec, 1.0f);
        fillVector(vec2, 2.0f);

        ComplexVector vec3 = vec + vec2;

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(6.0f * i, vec3[i].real());
            EXPECT_EQ(6.0f * i + 3.0f, vec3[i].imag());
        }

        vec3 += vec2;

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(10.0f * i, vec3[i].real());
            EXPECT_EQ(10.0f * i + 5.0f, vec3[i].imag());
        }
    }

    void testSubtractComplexFloat()
    {
        ComplexVector vec, vec2;
        fillVector(vec, 1.0f);
        fillVector(vec2, 2.0f);

        ComplexVector vec3 = vec - vec2;

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(-2.0f * i, vec3[i].real());
            EXPECT_EQ(-2.0f * i - 1.0f, vec3[i].imag());
        }

        vec3 -= vec2;

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(-6.0f * i, vec3[i].real());
            EXPECT_EQ(-6.0f * i - 3.0f, vec3[i].imag());
        }
    }

    void testMultiplyComplexFloat()
    {
        ComplexVector vec, vec2;
        fillVector(vec, 1.0f);
        fillVector(vec2, 2.0f);

        ComplexVector vec3 = vec * vec2;

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(-8.0f * i - 2.0f, vec3[i].real());
            EXPECT_EQ(16 * i * i + 8 * i, vec3[i].imag());
        }

        vec3 *= vec2;

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(-64.0f * i * i * i - 96 * i * i - 24 * i, vec3[i].real());
            EXPECT_EQ(64.0f * i * i * i - 24 * i - 4, vec3[i].imag());
        }
    }

    void testMultiplyConjugatedComplexFloat()
    {
        ComplexVector vec, vec2;
        fillVector(vec, 1.0f);
        fillVector(vec2, 2.0f);

        ComplexVector vec3 = multiplyConjugated(vec, vec2);

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(16.0f * i * i + 8 * i + 2, vec3[i].real());
            EXPECT_EQ(0, vec3[i].imag());
        }
    }

    void testConjugateComplexFloat()
    {
        ComplexVector vec;
        fillVector(vec, 1.0f);

        ComplexVector vec2 = conjugate(vec);

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(2.0f * i, vec2[i].real());
            EXPECT_EQ(-2.0f * i - 1.0f, vec2[i].imag());
        }
    }

    void testComplexConversions()
    {
        ComplexVector vec;
        fillVector(vec, 1.0f);

        auto re = real(vec), im = imag(vec);

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(2.0f * i, re[i]);
            EXPECT_EQ(2.0f * i + 1.0f, im[i]);
        }

        Vector re2, im2;
        ComplexVector vec2 = vec + ComplexVector(std::complex<float>(2 * ComplexVector::Elements, 2 * ComplexVector::Elements));
        splitComplex(vec, vec2, re2, im2);

        for (std::size_t i = 0; i < Vector::Elements; ++i)
        {
            EXPECT_EQ(2.0f * i, re2[i]);
            EXPECT_EQ(2.0f * i + 1.0f, im2[i]);
        }

        ComplexVector vec3, vec4;
        mergeComplex(re2, im2, vec3, vec4);

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(2.0f * i, vec3[i].real());
            EXPECT_EQ(2.0f * i + 1.0f, vec3[i].imag());
            EXPECT_EQ(2.0f * i + 2 * ComplexVector::Elements, vec4[i].real());
            EXPECT_EQ(2.0f * i + 1.0f + 2 * ComplexVector::Elements, vec4[i].imag());
        }
    }

    void testComplexFlip()
    {
        ComplexVector vec;
        fillVector(vec, 1.0f);

        ComplexVector vec2 = flip(vec);

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(2 * (ComplexVector::Elements - i - 1), vec2[i].real());
            EXPECT_EQ(2 * (ComplexVector::Elements - i - 1) + 1, vec2[i].imag());
        }
    }

    void testExtend()
    {
        typename Vector::HalfVectorType vec;
        fillVector(vec, 1.0f);

        auto vec2 = extendAdjacentAs<Vector>(vec);

        for (std::size_t i = 0; i < Vector::Elements; ++i)
        {
            EXPECT_EQ(1.0f * (i / 2), vec2[i]);
        }
    }

    void testAbs()
    {
        Vector vec;
        fillVector(vec, -1.0f);

        Vector vec2 = abs(vec);

        for (std::size_t i = 0; i < Vector::Elements; ++i)
        {
            EXPECT_EQ(1.0f * i, vec2[i]);
        }
    }

    void testFma()
    {
        Vector vec, vec2, vec3;
        fillVector(vec, 2.0f);
        fillVector(vec2, 3.0f);
        fillVector(vec3, 4.0f);
        Vector vec4 = fusedMultiplyAdd(vec, vec2, vec3);

        for (std::size_t i = 0; i < Vector::Elements; ++i)
        {
            EXPECT_EQ(6.0f * i * i + 4.0f * i, vec4[i]);
        }
    }

    void testFmaComplex()
    {
        ComplexVector vec, vec2, vec3;
        fillVector(vec, 2.0f);
        fillVector(vec2, 3.0f);
        fillVector(vec3, 4.0f);

        ComplexVector vec4 = fusedMultiplyAdd(vec, vec2, vec3);

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(-16.0f * i - 6.0f, vec4[i].real());
            EXPECT_EQ(48.0f * i * i + 32.0f * i + 4.0f, vec4[i].imag());
        }
    }

    void testFmaComplexLeft()
    {
        ComplexVector vec, vec3;
        Vector vec2;
        fillVector(vec, 2.0f);
        fillVector(vec2, 3.0f);
        fillVector(vec3, 4.0f);

        ComplexVector vec4 = fusedMultiplyAdd(vec, vec2, vec3);

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(24.0f * i * i + 8.0f * i, vec4[i].real());
            EXPECT_EQ(24.0f * i * i + 32 * i + 10.0f, vec4[i].imag());
        }
    }

    void testFmaComplexRight()
    {
        Vector vec;
        ComplexVector vec2, vec3;
        fillVector(vec, 2.0f);
        fillVector(vec2, 3.0f);
        fillVector(vec3, 4.0f);

        ComplexVector vec4 = fusedMultiplyAdd(vec, vec2, vec3);

        for (std::size_t i = 0; i < ComplexVector::Elements; ++i)
        {
            EXPECT_EQ(24.0f * i * i + 8.0f * i, vec4[i].real());
            EXPECT_EQ(24.0f * i * i + 32 * i + 10.0f, vec4[i].imag());
        }
    }

    void testSumFloat()
    {
        Vector vec(0);
        float vals[256] __attribute__((aligned(Vector::Alignment)));

        for (std::size_t i = 0; i < 256; ++i)
        {
            vals[i] = i;
        }

        for (std::size_t i = 0; i < 256; i += Vector::Elements)
        {
            vec += *reinterpret_cast<const Vector*>(&vals[i]);
        }

        float result = sum(vec);

        EXPECT_EQ(32640.0f, result);
    }

    void testSumComplexFloat()
    {
        ComplexVector vec(0);
        std::complex<float> vals[256] __attribute__((aligned(ComplexVector::Alignment)));
    
        for (std::size_t i = 0; i < 256; ++i)
        {
            vals[i] = std::complex<float>(i, i);
        }
    
        for (std::size_t i = 0; i < 256; i += ComplexVector::Elements)
        {
            vec += *reinterpret_cast<const ComplexVector*>(&vals[i]);
        }
    
        std::complex<float> result = sum(vec);
        EXPECT_EQ(std::complex<float>(32640.0f, 32640.f), result);
    }

    template <typename FilledVector>
    void fillVector(FilledVector& vec, float scale)
    {
        float vals[FilledVector::ByteSize] __attribute__((aligned(FilledVector::Alignment)));
        for (std::size_t i = 0; i < FilledVector::FlattenedType::Elements; ++i)
        {
            vals[i] = scale * i;
        }

        vec = *reinterpret_cast<FilledVector*>(&vals);
    }
};

} // anonymous namespace

DISABLE_WARNING_PUSH("-Wgnu-zero-variadic-macro-arguments")
TYPED_TEST_SUITE(SimdVectorTest, SimdByteSizes);
DISABLE_WARNING_POP()

TYPED_TEST(SimdVectorTest, AddFloat)
{
    this->testAddFloat();
}

TYPED_TEST(SimdVectorTest, SubtractFloat)
{
    this->testSubtractFloat();
}

TYPED_TEST(SimdVectorTest, MultiplyFloat)
{
    this->testMultiplyFloat();
}

TYPED_TEST(SimdVectorTest, AddComplexFloat)
{
    this->testAddComplexFloat();
}

TYPED_TEST(SimdVectorTest, SubtractComplexFloat)
{
    this->testSubtractComplexFloat();
}

TYPED_TEST(SimdVectorTest, MultiplyComplexFloat)
{
    this->testMultiplyComplexFloat();
}

TYPED_TEST(SimdVectorTest, MultiplyConjugatedComplexFloat)
{
    this->testMultiplyConjugatedComplexFloat();
}

TYPED_TEST(SimdVectorTest, ConjugateComplexFloat)
{
    this->testConjugateComplexFloat();
}


TYPED_TEST(SimdVectorTest, ComplexConversions)
{
    this->testComplexConversions();
}

TYPED_TEST(SimdVectorTest, ComplexFlip)
{
    this->testComplexFlip();
}

TYPED_TEST(SimdVectorTest, Extend)
{
    this->testExtend();
}

TYPED_TEST(SimdVectorTest, Abs)
{
    this->testAbs();
}

TYPED_TEST(SimdVectorTest, SumFloat)
{
    this->testSumFloat();
}

TYPED_TEST(SimdVectorTest, SumComplexFloat)
{
    this->testSumComplexFloat();
}

TYPED_TEST(SimdVectorTest, Fma)
{
    this->testFma();
}

TYPED_TEST(SimdVectorTest, FmaComplex)
{
    this->testFmaComplex();
}

TYPED_TEST(SimdVectorTest, FmaComplexLeft)
{
    this->testFmaComplexLeft();
}

TYPED_TEST(SimdVectorTest, FmaComplexRight)
{
    this->testFmaComplexRight();
}
