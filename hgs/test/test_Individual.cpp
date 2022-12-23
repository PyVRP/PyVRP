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
