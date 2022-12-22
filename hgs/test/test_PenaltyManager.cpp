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
}

// TODO test updating, penalty calculations, booster
