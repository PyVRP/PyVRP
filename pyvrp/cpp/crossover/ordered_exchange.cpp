// Nothing too special here in this file.
// - Change signature to resemble SREX signature
// - GetTour function
#include "Individual.h"
#include "Params.h"

#include <cassert>
#include <deque>
#include <fstream>
#include <numeric>
#include <vector>

using Parents = std::pair<Individual const *, Individual const *>;
using Offsets = std::pair<unsigned, unsigned>;

namespace
{
Offsets getStartEnd(int n, XorShift128 &rng)
{
    size_t start = rng.randint(n);
    size_t end = rng.randint(n);

    while (end == start)
        end = rng.randint(n);

    return std::make_pair(start, end);
}

Individual
doExchange(Parents const &parents, Params const &params, Offsets const &offsets)
{
    auto const &tour1 = parents.first->getTour();
    auto const &tour2 = parents.second->getTour();

    std::vector<int> offspringTour(params.nbClients);
    std::vector<bool> copied(params.nbClients + 1, false);

    auto const [start, end] = offsets;
    auto const idx2client = [&](size_t idx) { return idx % params.nbClients; };

    // Copy clients of first parent from start to end (with wrap-around)
    size_t insertPos = start;
    for (; idx2client(insertPos) != idx2client(end + 1); ++insertPos)
    {
        offspringTour[idx2client(insertPos)] = tour1[idx2client(insertPos)];
        copied[offspringTour[idx2client(insertPos)]] = true;  // mark as copied
    }

    // Fill the remaining clients in the order given by the second parent
    for (int idx = 1; idx <= params.nbClients; ++idx)
    {
        int const client = tour2[idx2client(end + idx)];
        if (!copied[client])
            offspringTour[idx2client(insertPos++)] = client;
    }

    return {&params, offspringTour};
}
}  // namespace

Individual
orderedExchange(Parents const &parents, Params const &params, XorShift128 &rng)
{
    // Performs another binary tournament with the crossover results. This is
    // not unlike ``Genetic::crossoverOX`` in the baseline.
    auto const slice1 = getStartEnd(params.nbClients, rng);
    auto const indiv1 = doExchange(parents, params, slice1);

    auto const slice2 = getStartEnd(params.nbClients, rng);
    auto const indiv2 = doExchange(parents, params, slice2);

    return std::min(indiv1, indiv2);
}
