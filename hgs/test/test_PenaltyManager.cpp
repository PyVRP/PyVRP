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

    // Booster no longer in scope, so penalties should return to normal.
    ASSERT_EQ(pm.twPenalty(1), 1);
    ASSERT_EQ(pm.loadPenalty(2), 1);  // 1 unit above capacity
}

TEST(PenaltyManagerTest, testCapacityPenaltyUpdateIncrease)
{
    PenaltyManager pm(1, 1, 1.1, 0.9, 0.5, 1, 1);

    // Within bandwidth, so penalty should not change.
    ASSERT_EQ(pm.loadPenalty(2), 1);
    pm.updateCapacityPenalty(0.5);
    ASSERT_EQ(pm.loadPenalty(2), 1);

    // Below targetFeasible, so should increase the capacityPenalty by +1
    // (normally to 1.1 due to penaltyIncrease, but we should not end up at the
    // same int).
    pm.updateCapacityPenalty(0.4);
    ASSERT_EQ(pm.loadPenalty(2), 2);

    // Now we start from a much bigger initial capacityPenalty. Here we want
    // the penalty to increase by 10% due to penaltyIncrease = 1.1, and +1 due
    // to double -> int.
    PenaltyManager pm2(100, 1, 1.1, 0.9, 0.5, 1, 1);
    ASSERT_EQ(pm2.loadPenalty(2), 100);
    pm2.updateCapacityPenalty(0.4);
    ASSERT_EQ(pm2.loadPenalty(2), 111);

    // Test if the penalty cannot increase beyond 1000, its maximum value.
    PenaltyManager pm3(1000, 1, 1.1, 0.9, 0.5, 1, 1);
    ASSERT_EQ(pm3.loadPenalty(2), 1000);
    pm3.updateCapacityPenalty(0.4);
    ASSERT_EQ(pm3.loadPenalty(2), 1000);
}

TEST(PenaltyManagerTest, testCapacityPenaltyUpdateDecrease)
{
    PenaltyManager pm(4, 1, 1.1, 0.9, 0.5, 1, 1);

    // Within bandwidth, so penalty should not change.
    ASSERT_EQ(pm.loadPenalty(2), 4);
    pm.updateCapacityPenalty(0.5);
    ASSERT_EQ(pm.loadPenalty(2), 4);

    // Above targetFeasible, so should decrease the capacityPenalty to 90%, and
    // -1 from the bounds check. So 0.9 * 4 = 3.6, 3.6 - 1 = 2.6, (int) 2.6 = 2
    pm.updateCapacityPenalty(0.6);
    ASSERT_EQ(pm.loadPenalty(2), 2);

    // Now we start from a much bigger initial capacityPenalty. Here we want
    // the penalty to decrease by 10% due to penaltyDecrease = 0.9, and -1 due
    // to double -> int.
    PenaltyManager pm2(100, 1, 1.1, 0.9, 0.5, 1, 1);
    ASSERT_EQ(pm2.loadPenalty(2), 100);
    pm2.updateCapacityPenalty(0.6);
    ASSERT_EQ(pm2.loadPenalty(2), 89);

    // Test if the penalty cannot decrease beyond 1, its minimum value.
    PenaltyManager pm3(1, 1, 1.1, 0.9, 0.5, 1, 1);
    ASSERT_EQ(pm3.loadPenalty(2), 1);
    pm3.updateCapacityPenalty(0.6);
    ASSERT_EQ(pm3.loadPenalty(2), 1);
}

TEST(PenaltyManagerTest, testTimeWarpPenaltyUpdateIncrease)
{
    PenaltyManager pm(1, 1, 1.1, 0.9, 0.5, 1, 1);

    // Within bandwidth, so penalty should not change.
    ASSERT_EQ(pm.twPenalty(1), 1);
    pm.updateCapacityPenalty(0.5);
    ASSERT_EQ(pm.twPenalty(1), 1);

    // Below targetFeasible, so should increase the timeWarpCapacity by +1
    // (normally to 1.1 due to penaltyIncrease, but we should not end up at the
    // same int).
    pm.updateTimeWarpPenalty(0.4);
    ASSERT_EQ(pm.twPenalty(1), 2);

    // Now we start from a much bigger initial timeWarpCapacity. Here we want
    // the penalty to increase by 10% due to penaltyIncrease = 1.1, and +1 due
    // to double -> int.
    PenaltyManager pm2(1, 100, 1.1, 0.9, 0.5, 1, 1);
    ASSERT_EQ(pm2.twPenalty(1), 100);
    pm2.updateTimeWarpPenalty(0.4);
    ASSERT_EQ(pm2.twPenalty(1), 111);

    // Test if the penalty cannot increase beyond 1000, its maximum value.
    PenaltyManager pm3(1, 1000, 1.1, 0.9, 0.5, 1, 1);
    ASSERT_EQ(pm3.twPenalty(1), 1000);
    pm3.updateTimeWarpPenalty(0.4);
    ASSERT_EQ(pm3.twPenalty(1), 1000);
}

TEST(PenaltyManagerTest, testTimeWarpPenaltyUpdateDecrease)
{
    PenaltyManager pm(1, 4, 1.1, 0.9, 0.5, 1, 1);

    // Within bandwidth, so penalty should not change.
    ASSERT_EQ(pm.twPenalty(1), 4);
    pm.updateTimeWarpPenalty(0.5);
    ASSERT_EQ(pm.twPenalty(1), 4);

    // Above targetFeasible, so should decrease the timeWarPenalty to 90%, and
    // -1 from the bounds check. So 0.9 * 4 = 3.6, 3.6 - 1 = 2.6, (int) 2.6 = 2
    pm.updateTimeWarpPenalty(0.6);
    ASSERT_EQ(pm.twPenalty(1), 2);

    // Now we start from a much bigger initial timeWarpCapacity. Here we want
    // the penalty to decrease by 10% due to penaltyDecrease = 0.9, and -1 due
    // to double -> int.
    PenaltyManager pm2(1, 100, 1.1, 0.9, 0.5, 1, 1);
    ASSERT_EQ(pm2.twPenalty(1), 100);
    pm2.updateTimeWarpPenalty(0.6);
    ASSERT_EQ(pm2.twPenalty(1), 89);

    // Test if the penalty cannot decrease beyond 1, its minimum value.
    PenaltyManager pm3(1, 1, 1.1, 0.9, 0.5, 1, 1);
    ASSERT_EQ(pm3.twPenalty(1), 1);
    pm3.updateTimeWarpPenalty(0.6);
    ASSERT_EQ(pm3.twPenalty(1), 1);
}
