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

TEST(ParamsFromFileContentTest, OkSmallInstance)
{
    auto const path = "data/OkSmall.txt";
    auto const params = Params::fromFile(Config{}, path);

    // From the DIMENSION, VEHICLES, and CAPACITY fields in the file.
    ASSERT_EQ(params.nbClients, 4);
    ASSERT_EQ(params.nbVehicles, 3);
    ASSERT_EQ(params.vehicleCapacity, 10);

    // From the NODE_COORD_SECTION in the file
    std::vector<std::pair<int, int>> expectedCoords = {
        {2334, 726},
        {226, 1297},
        {590, 530},
        {435, 718},
        {1191, 639},
    };

    ASSERT_EQ(params.nbClients + 1, expectedCoords.size());

    for (auto idx = 0; idx != 5; ++idx)
    {
        ASSERT_EQ(params.clients[idx].x, expectedCoords[idx].first);
        ASSERT_EQ(params.clients[idx].y, expectedCoords[idx].second);
    }

    // From the EDGE_WEIGHT_SECTION in the file
    std::vector<std::vector<int>> expectedDistances = {
        {0, 1544, 1944, 1931, 1476},
        {1726, 0, 1992, 1427, 1593},
        {1965, 1975, 0, 621, 1090},
        {2063, 1433, 647, 0, 818},
        {1475, 1594, 1090, 828, 0},
    };

    ASSERT_EQ(params.nbClients + 1, expectedDistances.size());

    for (auto i = 0; i != 5; ++i)
        for (auto j = 0; j != 5; ++j)
            ASSERT_EQ(params.dist(i, j), expectedDistances[i][j]);

    // From the DEMAND_SECTION in the file
    std::vector<int> expectedDemands = {0, 5, 5, 3, 5};
    ASSERT_EQ(params.nbClients + 1, expectedDemands.size());

    for (auto idx = 0; idx != 5; ++idx)
        ASSERT_EQ(params.clients[idx].demand, expectedDemands[idx]);

    // From the TIME_WINDOW_SECTION in the file
    std::vector<std::pair<int, int>> expectedTimeWindows = {
        {0, 45000},
        {15600, 22500},
        {12000, 19500},
        {8400, 15300},
        {12000, 19500},
    };

    ASSERT_EQ(params.nbClients + 1, expectedTimeWindows.size());

    for (auto idx = 0; idx != 5; ++idx)
    {
        ASSERT_EQ(params.clients[idx].twEarly, expectedTimeWindows[idx].first);
        ASSERT_EQ(params.clients[idx].twLate, expectedTimeWindows[idx].second);
    }

    // From the SERVICE_TIME_SECTION in the file
    std::vector<int> expectedServiceTimes = {0, 360, 360, 420, 360};
    ASSERT_EQ(params.nbClients + 1, expectedServiceTimes.size());

    for (auto idx = 0; idx != 5; ++idx)
        ASSERT_EQ(params.clients[idx].servDur, expectedServiceTimes[idx]);
}

TEST(ParamsFromFileContentTest, CVRPLIBEn22k4)  // instance from CVRPLIB
{
    auto const path = "data/E-n22-k4.vrp.txt";
    auto const params = Params::fromFile(Config{}, path);

    ASSERT_EQ(params.nbClients, 21);
    ASSERT_EQ(params.vehicleCapacity, 6000);

    // We have "k4" in the file name, but there's no VEHICLES field in the data
    // file itself, so the number of vehicles should default to the number of
    // clients, 21.
    ASSERT_EQ(params.nbVehicles, 21);

    ASSERT_EQ(params.clients[0].x, 145);  // depot location
    ASSERT_EQ(params.clients[0].y, 215);

    ASSERT_EQ(params.clients[1].x, 151);  // first customer
    ASSERT_EQ(params.clients[1].y, 264);

    // The data file specifies distances as 2D Euclidean. We take that and
    // should compute integer equivalents with up to one decimal precision.
    // For depot -> first customer:
    //      dX = 151 - 145 = 6
    //      dY = 264 - 215 = 49
    //      dist = sqrt(dX^2 + dY^2) = 49.37
    //      int(10 * dist) = 493
    ASSERT_EQ(params.dist(0, 1), 493);
    ASSERT_EQ(params.dist(1, 0), 493);

    // These fields are all missing from the data file, and should thus retain
    // their default values.
    for (auto idx = 0; idx <= params.nbClients; ++idx)
    {
        ASSERT_EQ(params.clients[idx].servDur, 0);
        ASSERT_EQ(params.clients[idx].twEarly, 0);
        ASSERT_EQ(params.clients[idx].twLate, INT_MAX);
        ASSERT_EQ(params.clients[idx].releaseTime, 0);
    }
}
