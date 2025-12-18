#ifndef PYVRP_SEARCH_SEARCHSPACE_H
#define PYVRP_SEARCH_SEARCHSPACE_H

#include "DynamicBitset.h"
#include "ProblemData.h"

#include <vector>

namespace pyvrp::search
{
/**
 * SearchSpace(data: ProblemData, neighbours: list[list[int]])
 *
 * Manages a search space for the local search. The search space is granular,
 * around the given neighbourhood, and uses the concept of promising clients
 * to determine which client's neighbourhoods to search.
 */
class SearchSpace
{
public:
    using Neighbours = std::vector<std::vector<size_t>>;

private:
    ProblemData const &data_;

    // Neighborhood restrictions: list of nearby clients for each client (size
    // numLocations, but nothing is stored for the depots!).
    Neighbours neighbours_;

    // Tracks clients that can likely be improved by local search operators.
    DynamicBitset promising_;

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
    void markPromising(size_t client);

    /**
     * Marks all clients as promising.
     */
    void markAllPromising();

    /**
     * Unmarks all clients as promising.
     */
    void unmarkAllPromising();
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SEARCHSPACE_H
