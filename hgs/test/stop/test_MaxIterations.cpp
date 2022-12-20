#include <gtest/gtest.h>

#include "MaxIterations.h"

#include <stdexcept>

TEST(MaxIterationsTest, ctor)
{
    // Zero iterations should not be possible.
    ASSERT_THROW(MaxIterations(0), std::invalid_argument);
}

TEST(MaxIterationsTest, stopsAfterMaxIterations)
{
    MaxIterations stop(100);

    for (auto iter = 0; iter != 100; ++iter)  // max iterations 100, so should
        ASSERT_FALSE(stop(0));                // not stop for 100 calls

    ASSERT_TRUE(stop(0));  // but now should
}
