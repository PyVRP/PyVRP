#include <gtest/gtest.h>

#include "Config.h"
#include "Individual.h"
#include "ProblemData.h"

TEST(IndividualTest, routeConstructorSortsByEmpty)
{
    Config config;
    auto const data = ProblemData::fromFile("data/OkSmall.txt");
    PenaltyManager pMngr(static_cast<int>(config.initialCapacityPenalty),
                         static_cast<int>(config.initialTimeWarpPenalty),
                         config.penaltyIncrease,
                         config.penaltyDecrease,
                         config.targetFeasible,
                         data.vehicleCapacity(),
                         static_cast<int>(config.repairBooster));

    Individual indiv{data, pMngr, {{3, 4}, {}, {1, 2}}};
    auto const &indivRoutes = indiv.getRoutes();

    // numRoutes() should show two non-empty routes. We passed-in three routes,
    // however, so indivRoutes.size() should not have changed.
    ASSERT_EQ(indiv.numRoutes(), 2);
    ASSERT_EQ(indivRoutes.size(), 3);

    // We expect Individual to sort the routes such that all non-empty routes
    // are in the lower indices.
    EXPECT_EQ(indivRoutes[0].size(), 2);
    EXPECT_EQ(indivRoutes[1].size(), 2);
    EXPECT_EQ(indivRoutes[2].size(), 0);
}

TEST(IndividualTest, routeConstructorThrows)
{
    Config config;
    auto const data = ProblemData::fromFile("data/OkSmall.txt");
    PenaltyManager pMngr(static_cast<int>(config.initialCapacityPenalty),
                         static_cast<int>(config.initialTimeWarpPenalty),
                         config.penaltyIncrease,
                         config.penaltyDecrease,
                         config.targetFeasible,
                         data.vehicleCapacity(),
                         static_cast<int>(config.repairBooster));

    ASSERT_EQ(data.numVehicles(), 3);

    // Two routes, three vehicles: should throw.
    ASSERT_THROW((Individual{data, pMngr, {{1, 2}, {4, 2}}}),
                 std::runtime_error);

    // Empty third route: should not throw.
    ASSERT_NO_THROW((Individual{data, pMngr, {{1, 2}, {4, 2}, {}}}));
}

TEST(IndividualTest, getNeighbours)
{
    Config config;
    auto const data = ProblemData::fromFile("data/OkSmall.txt");
    PenaltyManager pMngr(static_cast<int>(config.initialCapacityPenalty),
                         static_cast<int>(config.initialTimeWarpPenalty),
                         config.penaltyIncrease,
                         config.penaltyDecrease,
                         config.targetFeasible,
                         data.vehicleCapacity(),
                         static_cast<int>(config.repairBooster));

    Individual indiv{data, pMngr, {{3, 4}, {}, {1, 2}}};

    auto const &neighbours = indiv.getNeighbours();
    std::vector<std::pair<int, int>> expected = {
        {0, 0},  // 0: is depot
        {0, 2},  // 1: between depot (0) to 2
        {1, 0},  // 2: between 1 and depot (0)
        {0, 4},  // 3: between depot (0) and 4
        {3, 0},  // 4: between 3 and depot (0)
    };

    for (auto client = 0; client != 5; ++client)
        EXPECT_EQ(neighbours[client], expected[client]);
}

TEST(IndividualTest, feasibility)
{
    Config config;
    auto const data = ProblemData::fromFile("data/OkSmall.txt");
    PenaltyManager pMngr(static_cast<int>(config.initialCapacityPenalty),
                         static_cast<int>(config.initialTimeWarpPenalty),
                         config.penaltyIncrease,
                         config.penaltyDecrease,
                         config.targetFeasible,
                         data.vehicleCapacity(),
                         static_cast<int>(config.repairBooster));

    // This solution is infeasible due to both load and time window violations.
    Individual indiv{data, pMngr, {{1, 2, 3, 4}, {}, {}}};
    EXPECT_FALSE(indiv.isFeasible());

    // First route has total load 18, but vehicle capacity is only 10.
    EXPECT_TRUE(indiv.hasExcessCapacity());

    // Client 4 has TW [8400, 15300], but client 2 cannot be visited before
    // 15600, so there must be time warp on the single-route solution.
    EXPECT_TRUE(indiv.hasTimeWarp());

    // Let's try another solution that's actually feasible.
    Individual indiv2{data, pMngr, {{1, 2}, {3}, {4}}};
    EXPECT_TRUE(indiv2.isFeasible());
    EXPECT_FALSE(indiv2.hasExcessCapacity());
    EXPECT_FALSE(indiv2.hasTimeWarp());
}

TEST(IndividualCostTest, distance)
{
    Config config;
    auto const data = ProblemData::fromFile("data/OkSmall.txt");
    PenaltyManager pMngr(static_cast<int>(config.initialCapacityPenalty),
                         static_cast<int>(config.initialTimeWarpPenalty),
                         config.penaltyIncrease,
                         config.penaltyDecrease,
                         config.targetFeasible,
                         data.vehicleCapacity(),
                         static_cast<int>(config.repairBooster));

    Individual indiv{data, pMngr, {{1, 2}, {3}, {4}}};

    ASSERT_TRUE(indiv.isFeasible());

    // This individual is feasible, so cost should equal total distance
    // travelled.
    int dist = data.dist(0, 1, 2, 0) + data.dist(0, 3, 0) + data.dist(0, 4, 0);
    EXPECT_EQ(dist, indiv.cost());
}

TEST(IndividualCostTest, capacity)
{
    Config config;
    auto const data = ProblemData::fromFile("data/OkSmall.txt");
    PenaltyManager pMngr(static_cast<int>(config.initialCapacityPenalty),
                         static_cast<int>(config.initialTimeWarpPenalty),
                         config.penaltyIncrease,
                         config.penaltyDecrease,
                         config.targetFeasible,
                         data.vehicleCapacity(),
                         static_cast<int>(config.repairBooster));

    Individual indiv{data, pMngr, {{4, 3, 1, 2}, {}, {}}};

    ASSERT_TRUE(indiv.hasExcessCapacity());
    ASSERT_FALSE(indiv.hasTimeWarp());

    size_t load = 0;
    for (size_t idx = 0; idx <= data.numClients(); ++idx)
        load += data.client(idx).demand;

    auto const excessLoad = load - data.vehicleCapacity();
    ASSERT_GT(load, data.vehicleCapacity());
    EXPECT_EQ(excessLoad, 8);

    auto const loadPenalty = config.initialCapacityPenalty * excessLoad;
    int dist = data.dist(0, 4, 3, 1, 2, 0);

    // This individual is infeasible due to load violations, so the costs should
    // be distance + loadPenalty * excessLoad.
    EXPECT_EQ(dist + loadPenalty, indiv.cost());
}

TEST(IndividualCostTest, timeWarp)
{
    Config config;
    auto const data = ProblemData::fromFile("data/OkSmall.txt");
    PenaltyManager pMngr(static_cast<int>(config.initialCapacityPenalty),
                         static_cast<int>(config.initialTimeWarpPenalty),
                         config.penaltyIncrease,
                         config.penaltyDecrease,
                         config.targetFeasible,
                         data.vehicleCapacity(),
                         static_cast<int>(config.repairBooster));

    Individual indiv{data, pMngr, {{1, 3}, {2, 4}, {}}};

    ASSERT_FALSE(indiv.hasExcessCapacity());
    ASSERT_TRUE(indiv.hasTimeWarp());

    // There's only time warp on the first route: dist(0, 1) = 1'544, so we
    // arrive at 1 before its opening window of 15'600. Service (360) thus
    // starts at 15'600, and completes at 15'600 + 360. Then we drive
    // dist(1, 3) = 1'427, where we arrive after 15'300 (its closing time
    // window). This is where we incur time warp: we need to 'warp back' to
    // 15'300.
    int twR1 = 15'600 + 360 + 1'427 - 15'300;
    int twR2 = 0;
    int timeWarp = twR1 + twR2;
    int twPenalty = config.initialTimeWarpPenalty * timeWarp;
    int dist = data.dist(0, 1, 3, 0) + data.dist(0, 2, 4, 0);

    // This individual is infeasible due to time warp, so the costs should
    // be distance + twPenalty * timeWarp.
    EXPECT_EQ(dist + twPenalty, indiv.cost());

    // TODO test all time warp cases
}
