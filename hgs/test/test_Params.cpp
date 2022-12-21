#include <gtest/gtest.h>

#include "Config.h"
#include "Params.h"

TEST(ParamsFromFileTest, dimension)
{
    Config config;
    auto path = "test/data/ORTEC-VRPTW-ASYM-4c69f727-d1-n204-k12.txt";
    auto params = Params::fromFile(config, path);

    ASSERT_EQ(params.nbClients, 204);  // n204
    ASSERT_EQ(params.nbVehicles, 12);  // k12
}

TEST(ParamsFromFileTest, unknownEdgeWeightFmt)
{
    Config config;
    auto path = "test/data/UnknownEdgeWeightFmt.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}

TEST(ParamsFromFileTest, unknownEdgeWeightType)
{
    Config config;
    auto path = "test/data/UnknownEdgeWeightType.txt";
    ASSERT_THROW(Params::fromFile(config, path), std::runtime_error);
}
