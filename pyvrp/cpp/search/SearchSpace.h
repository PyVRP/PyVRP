#ifndef PYVRP_SEARCH_SEARCHSPACE_H
#define PYVRP_SEARCH_SEARCHSPACE_H

#include "DynamicBitset.h"
#include "ProblemData.h"

namespace pyvrp::search
{
/**
 * TODO
 */
class SearchSpace
{
    using Neighbours = std::vector<std::vector<size_t>>;

    // Neighborhood restrictions: list of nearby clients for each client (size
    // numLocations, but nothing is stored for the depots!)
    Neighbours neighbours_;

    // Tracks clients that can likely be improved by local search operators.
    DynamicBitset promising;

public:
    SearchSpace(ProblemData const &data, Neighbours neighbours);

    /**
     * Set the neighbourhood structure of this search space. For each client,
     * the neighbourhood structure is a vector of nearby clients. Depots have
     * no nearby clients.
     */
    void setNeighbours(Neighbours neighbours);

    /**
     * Returns the current neighbourhood structure.
     */
    Neighbours const &neighbours() const;

    /**
     * Returns the vector of neighbours for a given client.
     */
    std::vector<size_t> const &neighboursOf(size_t client) const;

    /**
     * Returns whether the given client is a promising evaluation candidate.
     */
    bool isPromising(size_t client) const;

    /**
     * Marks the given client as promising.
     */
    bool markPromising(size_t client);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SEARCHSPACE_H
