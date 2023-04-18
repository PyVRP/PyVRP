#include "alternating_exchange.h"

#include "crossover.h"

#include <unordered_set>
#include <vector>

using Client = int;
using ClientSet = std::unordered_set<Client>;
using Parents = std::pair<Individual const *, Individual const *>;
using Tour = std::vector<Client>;

Individual alternatingExchange(Parents const &parents,
                               Params const &params,
                               XorShift128 &rng)
{
    auto const &tourA = parents.first->getTour();
    auto const &tourB = parents.second->getTour();

    ClientSet seen;
    Tour tour;
    tour.reserve(params.nbClients);

    for (int idx = 0; idx != params.nbClients; ++idx)
    {
        if (!seen.contains(tourA[idx]))
        {
            tour.push_back(tourA[idx]);
            seen.insert(tourA[idx]);
        }

        if (!seen.contains(tourB[idx]))
        {
            tour.push_back(tourB[idx]);
            seen.insert(tourB[idx]);
        }
    }

    return Individual{&params, tour};
}
