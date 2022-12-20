#include <gtest/gtest.h>

#include "MaxRuntime.h"

#include <chrono>
#include <stdexcept>
#include <thread>

TEST(MaxRuntimeTest, ctor)
{
    ASSERT_THROW(MaxRuntime(-1), std::invalid_argument);
    ASSERT_THROW(MaxRuntime(0), std::invalid_argument);
}

TEST(MaxRuntimeTest, runsUntil)
{
    using namespace std::chrono_literals;

    MaxRuntime stop(.100);
    ASSERT_FALSE(stop(0));

    std::this_thread::sleep_for(100ms);
    ASSERT_TRUE(stop(0));
}
