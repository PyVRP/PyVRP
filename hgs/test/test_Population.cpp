#include <gtest/gtest.h>

#include "Population.h"

#include <iostream>

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

TEST(PopulationTest, addTriggersSurvivorSelection)
{
    auto const data = ProblemData::fromFile(Config{}, "data/OkSmall.txt");
    XorShift128 rng;
    Population pop(data, rng);

    // After construction, we should have minPopSize individuals.
    EXPECT_EQ(pop.size(), data.config.minPopSize);

    size_t infeasPops = pop.numInfeasible();
    size_t feasPops = pop.numFeasible();

    EXPECT_EQ(pop.size(), infeasPops + feasPops);

    while (true)  // keep adding feasible individuals until we are about to do
    {             // survivor selection.
        Individual indiv = {data, rng};

        if (indiv.isFeasible())
        {
            pop.add(indiv);
            feasPops++;

            EXPECT_EQ(pop.size(), infeasPops + feasPops);
            EXPECT_EQ(pop.numFeasible(), feasPops);
        }

        if (feasPops == data.config.minPopSize + data.config.generationSize)
            break;
    }

    // RNG is fixed, and this next individual is feasible. Since we now have a
    // feasible population size of minPopSize + generationSize, adding this new
    // individual should trigger survivor selection. Survivor selection reduces
    // the feasible sub-population to minPopSize, so the overall population is
    // just infeasPops + minPopSize
    Individual indiv = {data, rng};
    pop.add(indiv);

    ASSERT_TRUE(indiv.isFeasible());
    EXPECT_EQ(pop.numFeasible(), data.config.minPopSize);
    EXPECT_EQ(pop.size(), data.config.minPopSize + infeasPops);
}

// TODO test more add()

TEST(PopulationTest, select)
{
    // TODO
}

// TODO test more select()
