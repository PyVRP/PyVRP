#ifndef SUBPOPULATION_H
#define SUBPOPULATION_H

#include "Individual.h"
#include "ProblemData.h"
#include "diversity/diversity.h"

#include <iosfwd>
#include <stdexcept>
#include <vector>

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
    struct Item
    {
        using Proximity = std::vector<std::pair<Individual const *, const double>>;

        Individual const *individual;
        double fitness;
        Proximity proximity;
    };

    ProblemData const &data;
    DiversityMeasure const &divOp;
    PopulationParams const &params;
    std::vector<Item> items;

public:
    SubPopulation(ProblemData const &data,
                  DiversityMeasure const &divOp,
                  PopulationParams const &params);

    void add(Individual const *individual);

    size_t size() const;

    Individual const *operator[](size_t idx) const;

    void purge();

    void updateFitness();

    double avgDistanceClosest(size_t idx) const;
};

#endif  // SUBPOPULATION_H
