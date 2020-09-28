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

#include <hvylya/core/approx_trigonometry.h>

#include <hvylya/core/tests/common.h>

using namespace hvylya::core;

namespace {

template <typename SimdByteSize>
class ApproxTrigonometrySimdTest: public ::testing::Test
{
  public:
    typedef SimdVectorImpl<float, SimdByteSize::Value, Aligned> Vector;
    typedef SimdVectorImpl<std::complex<float>, SimdByteSize::Value, Aligned> ComplexVector;

    void testAtan2()
    {
        const int Samples = 256;
        const float Divisor = Samples / 16.0f;
        float max_dist = 0.0f;
        float avg_dist = 0.0f;
        std::size_t iterations = 0;
    
        for (int i = -Samples; i <= Samples; ++i)
        {
            for (int j = -Samples; j <= Samples; ++j)
            {
                std::complex<float> arg(i / Divisor, j / Divisor);
                ComplexVector vec(arg);
    
                auto result = ApproxTrigonometrySimdFromVector<ComplexVector>::atan2(vec, vec);
                auto result_normalized = ApproxTrigonometrySimdFromVector<ComplexVector>::normalized_atan2(vec, vec);
    
                float scalar_atan2_val = approx_atan2(arg.imag(), arg.real());
                float scalar_normalized_atan2_val = approx_normalized_atan2(arg.imag(), arg.real());
                float exact_atan2_val = std::atan2(arg.imag(), arg.real());
                float exact_normalized_atan2_val = 2.0f * std::atan2(arg.imag(), arg.real()) / float(M_PI);
    
                for (std::size_t k = 0; k < 2 * ComplexVector::Elements; ++k)
                {
                    EXPECT_NEAR(result[k], scalar_atan2_val, 1e-6);
                    EXPECT_NEAR(result_normalized[k], scalar_normalized_atan2_val, 1e-6);
    
                    EXPECT_NEAR(result[k], exact_atan2_val, 0.01f);
                    EXPECT_NEAR(result_normalized[k], exact_normalized_atan2_val, 0.01f);
    
                    float dist = std::abs(result_normalized[k] - exact_normalized_atan2_val);
                    max_dist = std::max(max_dist, dist);
                    avg_dist += dist;
                    ++iterations;
                }
            }
        }
    
        avg_dist /= iterations;
    
        EXPECT_NEAR(avg_dist, 0.0f, 0.0015f);
        EXPECT_NEAR(max_dist, 0.0f, 0.002f);
    }

    void testCos()
    {
        const int Range = 1024;
        const int Samples = 2 * Range + 1;

        float max_dist = 0.0f;
        float avg_dist = 0.0f;

        for (int i = -Range; i <= Range; ++i)
        {
            float arg = float(M_PI * i / Range);
            Vector vec(arg);

            auto result = ApproxTrigonometrySimdFromVector<Vector>::cos(vec);
            float scalar_cos_val = approx_cos(arg);
            float exact_cos_val = std::cos(arg);

            for (std::size_t k = 0; k < Vector::Elements; ++k)
            {
                EXPECT_FLOAT_EQ(result[k], scalar_cos_val);
                EXPECT_NEAR(result[k], exact_cos_val, 0.00015f);
                float dist = std::abs(result[k] - exact_cos_val);
                max_dist = std::max(max_dist, dist);
                avg_dist += dist;
            }
        }

        avg_dist /= Samples * Vector::Elements;

        EXPECT_NEAR(avg_dist, 0.0f, 0.00003f);
        EXPECT_NEAR(max_dist, 0.0f, 0.00015f);
    }

    void testSin()
    {
        const int Range = 1024;
        const int Samples = 2 * Range + 1;

        float max_dist = 0.0f;
        float avg_dist = 0.0f;

        for (int i = -Range; i <= Range; ++i)
        {
            float arg = float(M_PI * i / Range);
            Vector vec(arg);

            auto result = ApproxTrigonometrySimdFromVector<Vector>::sin(vec);
            float scalar_sin_val = approx_sin(arg);
            float exact_sin_val = std::sin(arg);

            for (std::size_t k = 0; k < Vector::Elements; ++k)
            {
                EXPECT_FLOAT_EQ(result[k], scalar_sin_val);
                EXPECT_NEAR(result[k], exact_sin_val, 0.00002f);
                float dist = std::abs(result[k] - exact_sin_val);
                max_dist = std::max(max_dist, dist);
                avg_dist += dist;
            }
        }

        avg_dist /= Samples * Vector::Elements;

        EXPECT_NEAR(avg_dist, 0.0f, 5e-6f);
        EXPECT_NEAR(max_dist, 0.0f, 2e-5f);
    }
};

} // anonyous namespace

DISABLE_WARNING_PUSH("-Wgnu-zero-variadic-macro-arguments")
TYPED_TEST_SUITE(ApproxTrigonometrySimdTest, SimdByteSizes);
DISABLE_WARNING_POP()

TYPED_TEST(ApproxTrigonometrySimdTest, Atan2Simd)
{
    this->testAtan2();
}

TYPED_TEST(ApproxTrigonometrySimdTest, CosSimd)
{
    this->testCos();
}

TYPED_TEST(ApproxTrigonometrySimdTest, SinSimd)
{
    this->testSin();
}
