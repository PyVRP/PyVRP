#include "diversity.h"

double pyvrp::diversity::brokenPairsDistance(pyvrp::Solution const &first,
                                             pyvrp::Solution const &second)
{
    auto const &fNeighbours = first.getNeighbours();
    auto const &sNeighbours = second.getNeighbours();

    // The neighbours vector contains the depot, so its size is always at least
    // one. Thus numClients >= 0.
    size_t const numClients = fNeighbours.size() - 1;
    size_t numBrokenPairs = 0;

    for (size_t j = 1; j <= numClients; j++)
    {
        auto const [fPred, fSucc] = fNeighbours[j];
        auto const [sPred, sSucc] = sNeighbours[j];

        // An edge pair (fPred, j) or (j, fSucc) from the first solution is
        // broken if it is not in the second solution.
        numBrokenPairs += fSucc != sSucc;
        numBrokenPairs += fPred != sPred;
    }

    // numBrokenPairs is at most 2n since we can count at most two broken edges
    // for each client. Here, we normalise the distance to [0, 1].
    return numBrokenPairs / (2. * std::max<size_t>(numClients, 1));
}
