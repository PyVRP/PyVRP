#include <gtest/gtest.h>

#include "XorShift128.h"

TEST(TestXorShift128, bounds)
{
    XorShift128 rng;
    ASSERT_EQ(rng.min(), 0);
    ASSERT_EQ(rng.max(), UINT_MAX);
}

TEST(TestXorShift128, call)
{
    XorShift128 rng1(42);
    ASSERT_EQ(rng1(), 2386648076);
    ASSERT_EQ(rng1(), 1236469084);

    XorShift128 rng2(43);
    ASSERT_EQ(rng2(), 2386648077);
    ASSERT_EQ(rng2(), 1236469085);
}

TEST(TestXorShift128, randint)
{
    XorShift128 rng(42);
    ASSERT_EQ(rng.randint(100), 2386648076 % 100);
    ASSERT_EQ(rng.randint(100), 1236469084 % 100);
}
