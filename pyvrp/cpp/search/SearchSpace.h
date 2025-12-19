#ifndef PYVRP_SEARCH_SEARCHSPACE_H
#define PYVRP_SEARCH_SEARCHSPACE_H

#include "DynamicBitset.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Route.h"

#include <vector>

namespace pyvrp::search
{
/**
 * SearchSpace(data: ProblemData, neighbours: list[list[int]])
 *
 * Manages a search space for the local search. The search space is granular,
 * around the given neighbourhood, and uses the concept of promising clients
 * to determine which client's neighbourhoods to search. It can also be used
 * to define a (randomised) search ordering for clients, routes, and vehicle
 * types.
 */
class SearchSpace
{
public:
    using Neighbours = std::vector<std::vector<size_t>>;

private:
    // Neighborhood restrictions: list of nearby clients for each client (size
    // numLocations, but nothing is stored for the depots!).
    Neighbours neighbours_;

    // Tracks clients that can likely be improved by local search operators.
    DynamicBitset promising_;

    // Client order used for node-based search.
    std::vector<size_t> clientOrder_;

    // Route order used for route-based search.
    std::vector<size_t> routeOrder_;

    // Vehicle type order - pairs of [veh type, offset] - used for empty route
    // search.
    std::vector<std::pair<size_t, size_t>> vehTypeOrder_;

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
     * Convenient overload for route nodes. Since this is typically used during
     * insert and removals, this method marks the given node and its direct
     * client neighbours as promising. The node must currently be in a route.
     * Does not mark depots.
     */
    void markPromising(Route::Node const *node);

    /**
     * Marks all clients as promising.
     */
    void markAllPromising();

    /**
     * Unmarks all clients as promising.
     */
    void unmarkAllPromising();

    /**
     * Returns a randomised order in which the client search space may be
     * traversed. This order remains unchanged until :meth:`~shuffle` is called.
     */
    std::vector<size_t> const &clientOrder() const;

    /**
     * Returns a randomised order in which the route search space may be
     * traversed. This order remains unchanged until :meth:`~shuffle` is called.
     */
    std::vector<size_t> const &routeOrder() const;

    /**
     * Returns a randomised order in which the vehicle type space may be
     * traversed. This order remains unchanged until :meth:`~shuffle` is called.
     */
    std::vector<std::pair<size_t, size_t>> const &vehTypeOrder() const;

    /**
     * Randomises the client, route, and vehicle type orders using the given
     * random number generator.
     */
    void shuffle(RandomNumberGenerator &rng);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SEARCHSPACE_H
