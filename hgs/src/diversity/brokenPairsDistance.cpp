#include "diversity.h"

double brokenPairsDistance(ProblemData const &data,
                           Individual const &first,
                           Individual const &second)
{
    auto const &fNeighbours = first.getNeighbours();
    auto const &sNeighbours = second.getNeighbours();

    int numBrokenPairs = 0;

    for (int j = 1; j <= data.nbClients; j++)
    {
        auto const [fPred, fSucc] = fNeighbours[j];
        auto const [sPred, sSucc] = sNeighbours[j];

        // An edge pair (fPred, j) or (j, fSucc) from the first solution is
        // broken if it is not in the second solution. Note that we double count
        // in this loop: we count each edge twice, for both j and for j +- 1.
        numBrokenPairs += fSucc != sSucc;
        numBrokenPairs += fPred != sPred;
    }

    // Average broken pairs distance, adjusted for double counting.
    return numBrokenPairs / (2. * data.nbClients);
}
