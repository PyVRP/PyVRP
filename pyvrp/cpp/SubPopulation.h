#ifndef SUBPOPULATION_H
#define SUBPOPULATION_H

#include "CostEvaluator.h"
#include "Individual.h"
#include "diversity/diversity.h"

#include <functional>
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
        if (lbDiversity < 0 || lbDiversity > 1)
            throw std::invalid_argument("lb_diversity must be in [0, 1].");

        if (ubDiversity < 0 || ubDiversity > 1)
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
    DiversityMeasure divOp;
    PopulationParams const &params;  // owned by Population, on the Python side

public:
    struct Item
    {
        using Proximity = std::vector<std::pair<double, Individual const *>>;

        PopulationParams const *params;

        // Note that this pointer is not owned by the Item - it is merely a
        // reference to memory owned and allocated by the SubPopulation this
        // item is part of. The SubPopulation remains responsible for managing
        // that memory.
        Individual const *individual;
        // Fitness should be used carefully: only directly after updateFitness
        // was called. At any other moment, it will be outdated.
        double fitness;
        Proximity proximity;

        double avgDistanceClosest() const;
    };

private:
    std::vector<Item> items;

    // Removes the element at the given iterator location from the items.
    void remove(std::vector<Item>::iterator const &iterator,
                CostEvaluator const &costEvaluator);

public:
    SubPopulation(DiversityMeasure divOp, PopulationParams const &params);

    ~SubPopulation();

    void add(Individual const *individual, CostEvaluator const &costEvaluator);

    std::vector<Item>::const_iterator cbegin() const;

    std::vector<Item>::const_iterator cend() const;

    size_t size() const;

    Item const &operator[](size_t idx) const;

    void purge(CostEvaluator const &costEvaluator);

    // Recomputes the fitness of all individuals maintained by this population.
    // This is called whenever an individual is added to, or removed from, the
    // population.
    void updateFitness(CostEvaluator const &costEvaluator);
};

#endif  // SUBPOPULATION_H
