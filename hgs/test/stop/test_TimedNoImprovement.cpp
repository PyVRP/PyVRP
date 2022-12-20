#include <gtest/gtest.h>

#include "TimedNoImprovement.h"

#include <chrono>
#include <thread>

TEST(TimedNoImprovementTest, oneIteration)
{
    TimedNoImprovement stop(1, 1.);

    // Tests if the criterion stops after a single iteration without
    // improvement. The calls below go from 1 -> 0 -> 0 (should stop).
    ASSERT_FALSE(stop(1));
    ASSERT_FALSE(stop(0));
    ASSERT_TRUE(stop(0));
}

TEST(TimedNoImprovementTest, nIterations)
{
    for (auto const n : {10, 100, 1000})
    {
        TimedNoImprovement stop(n, 1.);

        for (auto iter = 0; iter != n; ++iter)
            ASSERT_FALSE(stop(0));

        for (auto iter = 0; iter != n; ++iter)
            ASSERT_TRUE(stop(0));
    }
}

TEST(TimedNoImprovementTest, timeLimit)
{
    using namespace std::chrono_literals;

    TimedNoImprovement stop(10000,.100);
    ASSERT_FALSE(stop(0));

    std::this_thread::sleep_for(100ms);
    ASSERT_TRUE(stop(0));
}
