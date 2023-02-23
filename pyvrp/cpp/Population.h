#ifndef POPULATION_H
#define POPULATION_H

#include "Individual.h"
#include "ProblemData.h"
#include "diversity/diversity.h"

#include <iosfwd>
#include <stdexcept>

struct PopulationParams
{
    size_t minPopSize;
    size_t generationSize;
    size_t nbElite;
    size_t nbClose;
    double lbDiversity;
    double ubDiversity;

    PopulationParams(size_t minPopSize = 25,
                     size_t generationSize = 40,
                     size_t nbElite = 4,
                     size_t nbClose = 5,
                     double lbDiversity = 0.1,
                     double ubDiversity = 0.5)
        : minPopSize(minPopSize),
          generationSize(generationSize),
          nbElite(nbElite),
          nbClose(nbClose),
          lbDiversity(lbDiversity),
          ubDiversity(ubDiversity)
    {
        if (0 > lbDiversity || 1 < lbDiversity)
            throw std::invalid_argument("lb_diversity must be in [0, 1].");

        if (0 > ubDiversity || 1 < ubDiversity)
            throw std::invalid_argument("ub_diversity must be in [0, 1].");

        if (ubDiversity <= lbDiversity)
        {
            auto const msg = "ub_diversity <= lb_diversity not understood.";
            throw std::invalid_argument(msg);
        }
    }

    size_t maxPopSize() const { return minPopSize + generationSize; }
};

class SubPopulation
{
public:
    SubPopulation(ProblemData const &data,
                  DiversityMeasure const &divOp,
                  PopulationParams const &params);

    void add(Individual const &individual);

    void remove(Individual const &individual);

    void purge();

    void updateFitness();

    double avgDistanceClosest(size_t idx) const;
};

class Population
{
    Individual const *getBinaryTournament() const;

public:
    Population(ProblemData const &data,
               PenaltyManager const &penManager,
               XorShift128 const &rng,
               DiversityMeasure const &divOp,
               PopulationParams params = PopulationParams())

    SubPopulation const &feasibleSubPopulation() const;

    SubPopulation const &infeasibleSubPopulation() const;

    size_t size() const;

    void add(Individual const &individual);

    std::pair<Individual const *, Individual const *> select() const;

    void restart();
};

#endif  // POPULATION_H
