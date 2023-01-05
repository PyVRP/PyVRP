#include <gtest/gtest.h>

#include "diversity.h"

TEST(DiversityTest, brokenPairsDistance)
{
    auto const data = ProblemData::fromFile(Config{}, "data/OkSmall.txt");

    Individual indiv1{data, {{1, 2, 3, 4}, {}, {}}};
    Individual indiv2{data, {{1, 2}, {3}, {4}}};
    Individual indiv3{data, {{3}, {4, 1, 2}, {}}};

    // Compare indiv1 and indiv2. The two broken pairs are (2, 3) and (3, 4).
    EXPECT_DOUBLE_EQ(brokenPairsDistance(data, indiv1, indiv2), 0.5);
    EXPECT_DOUBLE_EQ(brokenPairsDistance(data, indiv2, indiv1), 0.5);

    // Compare indiv1 and indiv3. The three broken pairs are (0, 1), (2, 3),
    // and (3, 4).
    EXPECT_DOUBLE_EQ(brokenPairsDistance(data, indiv1, indiv3), 0.75);
    EXPECT_DOUBLE_EQ(brokenPairsDistance(data, indiv3, indiv1), 0.75);

    // Compare indiv2 and indiv3. The broken pair is (0, 1).
    EXPECT_DOUBLE_EQ(brokenPairsDistance(data, indiv2, indiv3), 0.25);
    EXPECT_DOUBLE_EQ(brokenPairsDistance(data, indiv3, indiv2), 0.25);
}
