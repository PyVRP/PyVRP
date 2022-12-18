#include <gtest/gtest.h>

TEST(TestIndividual, String)
{
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
}

TEST(TestIndividual, Math)
{
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}
