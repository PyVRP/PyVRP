#include <gtest/gtest.h>

#include "PenaltyManager.h"

#include <stdexcept>

TEST(PenaltyManagerTest, ctorThrowsInvalidArguments)
{
    // -1 penaltyIncrease
    ASSERT_THROW(PenaltyManager(1, 1, -1, 0.5, 0.5, 1, 1),
                 std::invalid_argument);

    // 0.5 penaltyIncrease
    ASSERT_THROW(PenaltyManager(1, 1, 0.5, 0.5, 0.5, 1, 1),
                 std::invalid_argument);

    // Boundary condition: 1 penaltyIncrease should be OK
    ASSERT_NO_THROW(PenaltyManager(1, 1, 1, 0.5, 0.5, 1, 1));

    // -1 penaltyDecrease
    ASSERT_THROW(PenaltyManager(1, 1, 1.5, -1, 0.5, 1, 1),
                 std::invalid_argument);

    // 2 penaltyDecrease
    ASSERT_THROW(PenaltyManager(1, 1, 1.5, 2, 0.5, 1, 1),
                 std::invalid_argument);

    // Boundary conditions: 0 and 1 penaltyDecrease should be OK
    ASSERT_NO_THROW(PenaltyManager(1, 1, 1, 1, 0.5, 1, 1));
    ASSERT_NO_THROW(PenaltyManager(1, 1, 1, 0, 0.5, 1, 1));

    // -1 targetFeasible
    ASSERT_THROW(PenaltyManager(1, 1, 1, 1, -1, 1, 1), std::invalid_argument);

    // 2 targetFeasible
    ASSERT_THROW(PenaltyManager(1, 1, 1, 1, 2, 1, 1), std::invalid_argument);

    // Boundary conditions: 0 and 1 targetFeasible should be OK
    ASSERT_NO_THROW(PenaltyManager(1, 1, 1, 1, 1, 1, 1));
    ASSERT_NO_THROW(PenaltyManager(1, 1, 1, 1, 0, 1, 1));

    // 0 repairBooster
    ASSERT_THROW(PenaltyManager(1, 1, 1, 1, 1, 1, 0), std::invalid_argument);

    // Boundary condition: repairBooster 1 should be OK
    ASSERT_NO_THROW(PenaltyManager(1, 1, 1, 1, 1, 1, 1));
}

TEST(PenaltyManagerTest, testLoadPenalty)
{
    PenaltyManager pm(2, 1, 1, 1, 1, 1, 1);

    EXPECT_EQ(pm.loadPenalty(0), 0);  // zero is below capacity
    EXPECT_EQ(pm.loadPenalty(1), 0);  // one is at capacity

    // Penalty per unit excess capacity is 2
    EXPECT_EQ(pm.loadPenalty(2), 2);  // 1 unit above capacity
    EXPECT_EQ(pm.loadPenalty(3), 4);  // 2 units above capacity

    // Penalty per unit excess capacity is 4
    PenaltyManager pm2(4, 1, 1, 1, 1, 1, 1);
    EXPECT_EQ(pm2.loadPenalty(2), 4);  // 1 unit above capacity
    EXPECT_EQ(pm2.loadPenalty(3), 8);  // 2 units above capacity
}

TEST(PenaltyManagerTest, testTimeWarpPenalty)
{
    PenaltyManager pm(1, 2, 1, 1, 1, 1, 1);

    // Penalty per unit time warp is 2
    EXPECT_EQ(pm.twPenalty(0), 0);
    EXPECT_EQ(pm.twPenalty(1), 2);
    EXPECT_EQ(pm.twPenalty(2), 4);

    // Penalty per unit excess capacity is 4
    PenaltyManager pm2(1, 4, 1, 1, 1, 1, 1);
    EXPECT_EQ(pm2.twPenalty(0), 0);
    EXPECT_EQ(pm2.twPenalty(1), 4);
    EXPECT_EQ(pm2.twPenalty(2), 8);
}

TEST(PenaltyManagerTest, testRepairBooster)
{
    PenaltyManager pm(1, 1, 1, 1, 1, 1, 5);

    ASSERT_EQ(pm.twPenalty(1), 1);
    ASSERT_EQ(pm.loadPenalty(2), 1);  // 1 unit above capacity

    // Block scope the booster lifetime. While the booster lives, the penalty
    // values are multiplied by the repairBooster term.
    {
        auto const booster = pm.getPenaltyBooster();

        ASSERT_EQ(pm.twPenalty(1), 5);
        ASSERT_EQ(pm.twPenalty(2), 10);

        ASSERT_EQ(pm.loadPenalty(2), 5);   // 1 unit above capacity
        ASSERT_EQ(pm.loadPenalty(3), 10);  // 2 units above capacity
    }

    ASSERT_EQ(pm.twPenalty(1), 1);
    ASSERT_EQ(pm.loadPenalty(2), 1);  // 1 unit above capacity
}

// TODO test updating
