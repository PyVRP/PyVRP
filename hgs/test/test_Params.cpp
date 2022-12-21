#include <gtest/gtest.h>

#include "Config.h"
#include "Params.h"

/**
 *  THE FIRST SET OF TESTS CHECK WHETHER INVALID FILES ARE PROPERLY REJECTED.
 **/

TEST(ParamsFromFileThrowsTest, UnknownEdgeWeightFmt)
{
    Config const config;
    auto const path = "data/UnknownEdgeWeightFmt.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, UnknownEdgeWeightType)
{
    Config const config;
    auto const path = "data/UnknownEdgeWeightType.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, UnknownFile)
{
    Config const config;
    auto const path = "somewhere that does not exist";
    ASSERT_THROW(Params::fromFile(config, path), std::invalid_argument);
}

TEST(ParamsFromFileThrowsTest, UnknownSectionInFile)
{
    Config const config;
    auto const path = "data/FileWithUnknownSection.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, WrongIdDepot)
{
    Config const config;
    auto const path = "data/DepotNotOne.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, WrongDepotEndIdentifier)
{
    Config const config;
    auto const path = "data/DepotSectionDoesNotEndInMinusOne.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, MoreThanOneDepot)
{
    Config const config;
    auto const path = "data/MoreThanOneDepot.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, NonZeroDepotServiceDuration)
{
    Config const config;
    auto const path = "data/NonZeroDepotServiceDuration.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, NonZeroDepotReleaseTime)
{
    Config const config;
    auto const path = "data/NonZeroDepotReleaseTime.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, NonZeroDepotOpenTimeWindow)
{
    Config const config;
    auto const path = "data/NonZeroDepotOpenTimeWindow.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, NonZeroDepotDemand)
{
    Config const config;
    auto const path = "data/NonZeroDepotDemand.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileThrowsTest, InconsistentTimeWindows)
{
    Config const config;
    auto const earlyEqLate = "data/TimeWindowOpenEqualToClose.txt";
    auto const earlyGtLate = "data/TimeWindowOpenLargerThanClose.txt";

    // Params::fromFile should throw when any twEarly >= twLate
    ASSERT_THROW(Params::fromFile(config, earlyEqLate), std::runtime_error);
    ASSERT_THROW(Params::fromFile(config, earlyGtLate), std::runtime_error);
}

/**
 * HERE START TESTS THAT CHECK CONTENT, NOT JUST WHETHER INVALID FILES ARE
 * PROPERLY REJECTED.
 **/

TEST(ParamsFromFileContentTest, dimension)
{
    Config const config;
    auto const path = "data/ORTEC-VRPTW-ASYM-4c69f727-d1-n204-k12.txt";
    auto const params = Params::fromFile(config, path);

    ASSERT_EQ(params.nbClients, 204);  // n204
    ASSERT_EQ(params.nbVehicles, 12);  // k12
}
