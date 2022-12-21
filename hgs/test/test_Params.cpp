#include <gtest/gtest.h>

#include "Config.h"
#include "Params.h"

/**
 *  THE FIRST SET OF TESTS CHECK WHETHER INVALID FILES ARE PROPERLY REJECTED.
 **/

TEST(ParamsFromFileThrowsTest, UnknownEdgeWeightFmt)
{
    auto const path = "data/UnknownEdgeWeightFmt.txt";
    ASSERT_THROW(Params::fromFile(Config{}, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, UnknownEdgeWeightType)
{
    auto const path = "data/UnknownEdgeWeightType.txt";
    ASSERT_THROW(Params::fromFile(Config{}, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, UnknownFile)
{
    auto const path = "somewhere that does not exist";
    ASSERT_THROW(Params::fromFile(Config{}, path), std::invalid_argument);
}

TEST(ParamsFromFileThrowsTest, UnknownSectionInFile)
{
    auto const path = "data/FileWithUnknownSection.txt";
    ASSERT_THROW(Params::fromFile(Config{}, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, WrongIdDepot)
{
    auto const path = "data/DepotNotOne.txt";
    ASSERT_THROW(Params::fromFile(Config{}, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, WrongDepotEndIdentifier)
{
    auto const path = "data/DepotSectionDoesNotEndInMinusOne.txt";
    ASSERT_THROW(Params::fromFile(Config{}, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, MoreThanOneDepot)
{
    auto const path = "data/MoreThanOneDepot.txt";
    ASSERT_THROW(Params::fromFile(Config{}, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, NonZeroDepotServiceDuration)
{
    auto const path = "data/NonZeroDepotServiceDuration.txt";
    ASSERT_THROW(Params::fromFile(Config{}, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, NonZeroDepotReleaseTime)
{
    auto const path = "data/NonZeroDepotReleaseTime.txt";
    ASSERT_THROW(Params::fromFile(Config{}, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, NonZeroDepotOpenTimeWindow)
{
    auto const path = "data/NonZeroDepotOpenTimeWindow.txt";
    ASSERT_THROW(Params::fromFile(Config{}, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, NonZeroDepotDemand)
{
    auto const path = "data/NonZeroDepotDemand.txt";
    ASSERT_THROW(Params::fromFile(Config{}, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, InconsistentTimeWindows)
{
    auto const earlyEqLate = "data/TimeWindowOpenEqualToClose.txt";
    auto const earlyGtLate = "data/TimeWindowOpenLargerThanClose.txt";

    // Params::fromFile should throw when any twEarly >= twLate
    ASSERT_THROW(Params::fromFile(Config{}, earlyEqLate), std::runtime_error);
    ASSERT_THROW(Params::fromFile(Config{}, earlyGtLate), std::runtime_error);
}

/**
 * HERE START TESTS THAT CHECK CONTENT, NOT JUST WHETHER INVALID FILES ARE
 * PROPERLY REJECTED.
 **/

TEST(ParamsFromFileContentTest, DimensionsSmallInstance)
{
    // Tests if Params::fromFile correctly reads the dimensions (# clients,
    // # vehicles, and vehicle capacity) for the small instance.
    auto const path = "data/OkSmall.txt";
    auto const params = Params::fromFile(Config{}, path);

    ASSERT_EQ(params.nbClients, 4);
    ASSERT_EQ(params.nbVehicles, 3);
    ASSERT_EQ(params.vehicleCapacity, 10);
}

TEST(ParamsFromFileContentTest, CoordinatesSmallInstance)
{
    auto const path = "data/OkSmall.txt";
    auto const params = Params::fromFile(Config{}, path);

    // From the NODE_COORD_SECTION in the file
    std::vector<std::pair<int, int>> expected = {
        {2334, 726},
        {226, 1297},
        {590, 530},
        {435, 718},
        {1191, 639},
    };

    ASSERT_EQ(params.nbClients + 1, expected.size());

    for (auto idx = 0; idx != 5; ++idx)
    {
        ASSERT_EQ(params.clients[idx].x, expected[idx].first);
        ASSERT_EQ(params.clients[idx].y, expected[idx].second);
    }
}

TEST(ParamsFromFileContentTest, EdgeWeightsSmallInstance)
{
    auto const path = "data/OkSmall.txt";
    auto const params = Params::fromFile(Config{}, path);

    // From the EDGE_WEIGHT_SECTION in the file
    std::vector<std::vector<int>> expected = {
        {0, 1544, 1944, 1931, 1476},
        {1726, 0, 1992, 1427, 1593},
        {1965, 1975, 0, 621, 1090},
        {2063, 1433, 647, 0, 818},
        {1475, 1594, 1090, 828, 0},
    };

    ASSERT_EQ(params.nbClients + 1, expected.size());

    for (auto i = 0; i != 5; ++i)
        for (auto j = 0; j != 5; ++j)
            ASSERT_EQ(params.dist(i, j), expected[i][j]);
}

TEST(ParamsFromFileContentTest, DemandsSmallInstance)
{
    auto const path = "data/OkSmall.txt";
    auto const params = Params::fromFile(Config{}, path);

    // From the DEMAND_SECTION in the file
    std::vector<int> expected = {0, 5, 5, 3, 5};
    ASSERT_EQ(params.nbClients + 1, expected.size());

    for (auto idx = 0; idx != 5; ++idx)
        ASSERT_EQ(params.clients[idx].demand, expected[idx]);
}

TEST(ParamsFromFileContentTest, TimeWindowsSmallInstance)
{
    auto const path = "data/OkSmall.txt";
    auto const params = Params::fromFile(Config{}, path);

    // From the TIME_WINDOW_SECTION in the file
    std::vector<std::pair<int, int>> expected = {
        {0, 45000},
        {15600, 22500},
        {12000, 19500},
        {8400, 15300},
        {12000, 19500},
    };

    ASSERT_EQ(params.nbClients + 1, expected.size());

    for (auto idx = 0; idx != 5; ++idx)
    {
        ASSERT_EQ(params.clients[idx].twEarly, expected[idx].first);
        ASSERT_EQ(params.clients[idx].twLate, expected[idx].second);
    }
}

TEST(ParamsFromFileContentTest, ServiceTimesSmallInstance)
{
    auto const path = "data/OkSmall.txt";
    auto const params = Params::fromFile(Config{}, path);

    // From the SERVICE_TIME_SECTION in the file
    std::vector<int> expected = {0, 360, 360, 420, 360};
    ASSERT_EQ(params.nbClients + 1, expected.size());

    for (auto idx = 0; idx != 5; ++idx)
        ASSERT_EQ(params.clients[idx].servDur, expected[idx]);
}
