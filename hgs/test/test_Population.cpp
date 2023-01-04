#include <gtest/gtest.h>

#include "Population.h"

TEST(PopulationTest, ctor)
{
    auto const data = ProblemData::fromFile(Config{}, "data/OkSmall.txt");
    XorShift128 rng;
    Population pop(data, rng);

    // Test that after construction, the population indeed consists of
    // minPopSize individuals.
    EXPECT_EQ(pop.size(), data.config.minPopSize);
}

// TODO additional property tests?

TEST(PopulationTest, addTriggersSurvivalSelection)
{
    // TODO
}

// TODO test more add()

TEST(PopulationTest, select)
{
    // TODO
}

// TODO test more select()
