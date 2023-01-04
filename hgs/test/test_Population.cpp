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

TEST(PopulationTest, addUpdatesBestFoundSolution)
{
    Config config;
    config.minPopSize = 0;

    auto const data = ProblemData::fromFile(config, "data/OkSmall.txt");
    XorShift128 rng(INT_MAX);
    Population pop(data, rng);

    // Should not have added any individuals to the population pool. The 'best'
    // individual, however, has already been initialised, with a random
    // individual.
    ASSERT_EQ(pop.size(), config.minPopSize);

    // This random individual is feasible and has cost 9240.
    auto const &best1 = pop.getBestFound();
    EXPECT_EQ(best1.cost(), 9240);
    EXPECT_TRUE(best1.isFeasible());

    // We now add a new, better solution to the population.
    pop.add({data, {{3, 2}, {1, 4}, {}}});

    // This new solution is feasible and has cost 9155, so adding it to the
    // population should replace the best found solution.
    auto const &best2 = pop.getBestFound();
    EXPECT_EQ(best2.cost(), 9155);
    EXPECT_TRUE(best2.isFeasible());
}

// TODO test more add()

TEST(PopulationTest, select)
{
    // TODO
}

// TODO test more select()
