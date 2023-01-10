#include <gtest/gtest.h>

#include "Population.h"

TEST(PopulationTest, ctor)
{
    Config config;
    auto const data = ProblemData::fromFile(config, "data/OkSmall.txt");
    PenaltyManager pMngr(static_cast<int>(config.initialCapacityPenalty),
                         static_cast<int>(config.initialTimeWarpPenalty),
                         config.penaltyIncrease,
                         config.penaltyDecrease,
                         config.targetFeasible,
                         data.vehicleCapacity,
                         static_cast<int>(config.repairBooster));
    XorShift128 rng;
    Population pop(data,
                   pMngr,
                   rng,
                   brokenPairsDistance,
                   config.minPopSize,
                   config.generationSize,
                   config.nbElite,
                   config.nbClose,
                   config.lbDiversity,
                   config.ubDiversity);

    // Test that after construction, the population indeed consists of
    // minPopSize individuals.
    EXPECT_EQ(pop.size(), data.config.minPopSize);
}

// TODO additional property tests?

TEST(PopulationTest, addTriggersPurge)
{
    Config config;
    auto const data = ProblemData::fromFile(config, "data/OkSmall.txt");
    PenaltyManager pMngr(static_cast<int>(config.initialCapacityPenalty),
                         static_cast<int>(config.initialTimeWarpPenalty),
                         config.penaltyIncrease,
                         config.penaltyDecrease,
                         config.targetFeasible,
                         data.vehicleCapacity,
                         static_cast<int>(config.repairBooster));
    XorShift128 rng;
    Population pop(data,
                   pMngr,
                   rng,
                   brokenPairsDistance,
                   config.minPopSize,
                   config.generationSize,
                   config.nbElite,
                   config.nbClose,
                   config.lbDiversity,
                   config.ubDiversity);

    // After construction, we should have minPopSize individuals.
    EXPECT_EQ(pop.size(), data.config.minPopSize);

    size_t infeasPops = pop.numInfeasible();
    size_t feasPops = pop.numFeasible();

    EXPECT_EQ(pop.size(), infeasPops + feasPops);

    while (true)  // keep adding feasible individuals until we are about to do
    {             // survivor selection.
        Individual indiv = {data, pMngr, rng};

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
    // just infeasPops + minPopSize.
    Individual indiv = {data, pMngr, rng};
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
    PenaltyManager pMngr(static_cast<int>(config.initialCapacityPenalty),
                         static_cast<int>(config.initialTimeWarpPenalty),
                         config.penaltyIncrease,
                         config.penaltyDecrease,
                         config.targetFeasible,
                         data.vehicleCapacity,
                         static_cast<int>(config.repairBooster));

    XorShift128 rng(2'147'483'647);
    Population pop(data,
                   pMngr,
                   rng,
                   brokenPairsDistance,
                   config.minPopSize,
                   config.generationSize,
                   config.nbElite,
                   config.nbClose,
                   config.lbDiversity,
                   config.ubDiversity);

    // Should not have added any individuals to the population pool. The 'best'
    // individual, however, has already been initialised, with a random
    // individual.
    ASSERT_EQ(pop.size(), config.minPopSize);

    // This random individual is feasible and has cost 9'339.
    auto const &best1 = pop.getBestFound();
    EXPECT_EQ(best1.cost(), 9'339);
    EXPECT_TRUE(best1.isFeasible());

    // We now add a new, better solution to the population.
    pop.add({data, pMngr, {{3, 2}, {1, 4}, {}}});

    // This new solution is feasible and has cost 9'155, so adding it to the
    // population should replace the best found solution.
    auto const &best2 = pop.getBestFound();
    EXPECT_EQ(best2.cost(), 9'155);
    EXPECT_TRUE(best2.isFeasible());
}

// TODO test more add() - fitness, duplicate, purge

TEST(PopulationTest, selectReturnsTheSameParentsIfNoOtherOption)
{
    Config config;
    config.minPopSize = 0;

    auto const data = ProblemData::fromFile(config, "data/OkSmall.txt");
    PenaltyManager pMngr(static_cast<int>(config.initialCapacityPenalty),
                         static_cast<int>(config.initialTimeWarpPenalty),
                         config.penaltyIncrease,
                         config.penaltyDecrease,
                         config.targetFeasible,
                         data.vehicleCapacity,
                         static_cast<int>(config.repairBooster));
    XorShift128 rng;
    Population pop(data,
                   pMngr,
                   rng,
                   brokenPairsDistance,
                   config.minPopSize,
                   config.generationSize,
                   config.nbElite,
                   config.nbClose,
                   config.lbDiversity,
                   config.ubDiversity);

    ASSERT_EQ(pop.size(), 0);

    Individual indiv1 = {data, pMngr, {{3, 2}, {1, 4}, {}}};
    pop.add(indiv1);

    {
        // We added a single individual, so we should now get the same parent
        // twice.
        auto const parents = pop.select();
        EXPECT_EQ(parents.first, parents.second);
    }

    // Now we add another, different parent.
    Individual indiv2 = {data, pMngr, {{3, 2}, {1}, {4}}};
    pop.add(indiv2);

    {
        // We should now not select the same parents again (it's not impossible,
        // but unlikely), because two different parents are available.
        auto const parents = pop.select();
        EXPECT_NE(parents.first, parents.second);
    }
}

// TODO test more select() - diversity, feas/infeas pairs
