#include <gtest/gtest.h>

#include "Config.h"
#include "Individual.h"
#include "ProblemData.h"

TEST(IndividualTest, routeConstructorSortsByEmpty)
{
    auto const data = ProblemData::fromFile(Config{}, "data/OkSmall.txt");
    std::vector<std::vector<int>> const routes = {
        {3, 4},
        {},
        {1, 2},
    };

    Individual indiv{data, routes};
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
    auto const data = ProblemData::fromFile(Config{}, "data/OkSmall.txt");
    ASSERT_EQ(data.nbVehicles, 3);

    // Two routes, three vehicles: should throw.
    ASSERT_THROW((Individual{data, {{1, 2}, {4, 2}}}), std::runtime_error);

    // Empty third route: should not throw.
    ASSERT_NO_THROW((Individual{data, {{1, 2}, {4, 2}, {}}}));
}

TEST(IndividualTest, getNeighbours)
{
    auto const data = ProblemData::fromFile(Config{}, "data/OkSmall.txt");
    std::vector<std::vector<int>> const routes = {
        {3, 4},
        {},
        {1, 2},
    };

    Individual indiv{data, routes};
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
    auto const data = ProblemData::fromFile(Config{}, "data/OkSmall.txt");

    // This solution is infeasible due to both load and time window violations.
    std::vector<std::vector<int>> const routes = {{1, 2, 3, 4}, {}, {}};
    Individual indiv{data, routes};
    EXPECT_FALSE(indiv.isFeasible());

    // First route has total load 18, but vehicle capacity is only 10.
    EXPECT_TRUE(indiv.hasExcessCapacity());

    // Client 4 has TW [8400, 15300], but client 2 cannot be visited before
    // 15600, so there must be time warp on the single-route solution.
    EXPECT_TRUE(indiv.hasTimeWarp());

    // Let's try another solution that's actually feasible.
    std::vector<std::vector<int>> const routes2 = {{1, 2}, {3}, {4}};
    Individual indiv2{data, routes2};
    EXPECT_TRUE(indiv2.isFeasible());
    EXPECT_FALSE(indiv2.hasExcessCapacity());
    EXPECT_FALSE(indiv2.hasTimeWarp());
}

TEST(IndividualTest, brokenPairsDistance)
{
    auto const data = ProblemData::fromFile(Config{}, "data/OkSmall.txt");

    std::vector<std::vector<int>> const routes1 = {{1, 2, 3, 4}, {}, {}};
    Individual indiv1{data, routes1};

    std::vector<std::vector<int>> const routes2 = {{1, 2}, {3}, {4}};
    Individual indiv2{data, routes2};

    // Due to clients 2 and 3: their successors differ between routes, and
    // they're not at the ends of a route in indiv1.
    EXPECT_EQ(indiv1.brokenPairsDistance(&indiv2), 2);

    std::vector<std::vector<int>> const routes3 = {{3}, {4, 1, 2}, {}};
    Individual indiv3{data, routes3};

    // Due to 1 not being next to a depot in indiv3, and 2 and 3 having
    // different successors between routes.
    EXPECT_EQ(indiv1.brokenPairsDistance(&indiv3), 3);
}

// TODO test cost computation
