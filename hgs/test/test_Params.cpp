#include <gtest/gtest.h>

#include "Config.h"
#include "Params.h"

TEST(ParamsFromFileTest, dimension)
{
    Config const config;
    auto const path = "data/ORTEC-VRPTW-ASYM-4c69f727-d1-n204-k12.txt";
    auto const params = Params::fromFile(config, path);

    ASSERT_EQ(params.nbClients, 204);  // n204
    ASSERT_EQ(params.nbVehicles, 12);  // k12
}

TEST(ParamsFromFileTest, throwsUnknownEdgeWeightFmt)
{
    Config const config;
    auto const path = "data/UnknownEdgeWeightFmt.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileTest, throwsUnknownEdgeWeightType)
{
    Config const config;
    auto const path = "data/UnknownEdgeWeightType.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileTest, throwsUnknownFile)
{
    Config const config;
    auto const path = "somewhere that does not exist";
    ASSERT_THROW(Params::fromFile(config, path), std::invalid_argument);
}

TEST(ParamsFromFileTest, throwsWrongIdDepot)
{
    Config const config;
    auto const path = "data/DepotNotOne.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileTest, throwsWrongDepotEndIdentifier)
{
    Config const config;
    auto const path = "data/DepotSectionDoesNotEndInMinusOne.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileTest, throwsMoreThanOneDepot)
{
    Config const config;
    auto const path = "data/MoreThanOneDepot.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileTest, throwsNonZeroDepotServiceDuration)
{
    Config const config;
    auto const path = "data/NonZeroDepotServiceDuration.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileTest, throwsNonZeroDepotReleaseTime)
{
    Config const config;
    auto const path = "data/NonZeroDepotReleaseTime.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileTest, throwsNonZeroDepotOpenTimeWindow)
{
    Config const config;
    auto const path = "data/NonZeroDepotOpenTimeWindow.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileTest, throwsNonZeroDepotDemand)
{
    Config const config;
    auto const path = "data/NonZeroDepotDemand.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}
