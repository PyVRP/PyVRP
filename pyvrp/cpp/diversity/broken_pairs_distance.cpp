#include "diversity.h"

#include <limits>

double pyvrp::diversity::brokenPairsDistance(pyvrp::Solution const &first,
                                             pyvrp::Solution const &second)
{
    auto const &fNeighbours = first.neighbours();
    auto const &sNeighbours = second.neighbours();

    size_t const numClients = fNeighbours.size();
    size_t numBrokenPairs = 0;

    for (size_t client = 0; client != numClients; client++)
    {
        // Large default in case a location is not in one of the solutions.
        size_t constexpr max = std::numeric_limits<size_t>::max();
        std::pair<size_t, size_t> constexpr unassigned = {max, max};

        auto const [fPred, fSucc] = fNeighbours[client].value_or(unassigned);
        auto const [sPred, sSucc] = sNeighbours[client].value_or(unassigned);

        // An edge pair (fPred, client) or (client, fSucc) from the first
        // solution is broken if it is not in the second solution.
        numBrokenPairs += fSucc != sSucc;
        numBrokenPairs += fPred != sPred;
    }

    // numBrokenPairs is at most 2n since we can count at most two broken edges
    // for each location. Here, we normalise the distance to [0, 1].
    return numBrokenPairs / (2. * numClients);
}
