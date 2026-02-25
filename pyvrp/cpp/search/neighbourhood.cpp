#include "neighbourhood.h"

#include <stdexcept>

using pyvrp::search::NeighbourhoodParams;

NeighbourhoodParams::NeighbourhoodParams(double weightWaitTime,
                                         double weightTimeWarp,
                                         size_t numNeighbours,
                                         bool symmetricProximity,
                                         bool symmetricNeighbours)
    : weightWaitTime(weightWaitTime),
      weightTimeWarp(weightTimeWarp),
      numNeighbours(numNeighbours),
      symmetricProximity(symmetricProximity),
      symmetricNeighbours(symmetricNeighbours)
{
    if (numNeighbours == 0)
        throw std::invalid_argument("num_neighbours <= 0 not understood.");
}

std::vector<std::vector<size_t>> pyvrp::search::computeNeighbours(
    [[maybe_unused]] pyvrp::ProblemData const &data,
    [[maybe_unused]] NeighbourhoodParams const &params)
{
    // TODO
    return {};
}
