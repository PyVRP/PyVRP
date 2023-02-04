#include "diversity.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

double brokenPairsDistance(ProblemData const &data,
                           Individual const &first,
                           Individual const &second)
{
    auto const &fNeighbours = first.getNeighbours();
    auto const &sNeighbours = second.getNeighbours();

    int numBrokenPairs = 0;

    for (size_t j = 1; j <= data.numClients(); j++)
    {
        auto const [fPred, fSucc] = fNeighbours[j];
        auto const [sPred, sSucc] = sNeighbours[j];

        // An edge pair (fPred, j) or (j, fSucc) from the first solution is
        // broken if it is not in the second solution.
        numBrokenPairs += fSucc != sSucc;
        numBrokenPairs += fPred != sPred;
    }

    // numBrokenPairs is at most 2n: for each client, since we can count at
    // most two broken edges in the loop above. Here, we normalise the distance
    // to [0, 1].
    return numBrokenPairs / (2. * data.numClients());
}

PYBIND11_MODULE(broken_pairs_distance, m)
{
    m.def("broken_pairs_distance",
          &brokenPairsDistance,
          py::arg("data"),
          py::arg("first"),
          py::arg("second"));
}
