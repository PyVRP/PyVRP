#include <gtest/gtest.h>

#include "NoImprovement.h"

#include <stdexcept>

TEST(NoImprovementTest, ctor)
{
    ASSERT_THROW(NoImprovement(0), std::invalid_argument);
}

TEST(NoImprovementTest, oneIteration)
{
    NoImprovement stop(1);

    // Tests if the criterion stops after a single iteration without
    // improvement. The calls below go from 1 -> 0 -> 0 (should stop).
    ASSERT_FALSE(stop(1));
    ASSERT_FALSE(stop(0));
    ASSERT_TRUE(stop(0));
}

TEST(NoImprovementTest, nIterations)
{
    for (auto const n : {10, 100, 1000})
    {
        NoImprovement stop(n);

        for (auto iter = 0; iter != n; ++iter)
            ASSERT_FALSE(stop(0));

        for (auto iter = 0; iter != n; ++iter)
            ASSERT_TRUE(stop(0));
    }
}
