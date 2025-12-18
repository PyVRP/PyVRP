#ifndef PYVRP_SEARCH_SOLUTION_H
#define PYVRP_SEARCH_SOLUTION_H

#include "../Solution.h"  // pyvrp::Solution
#include "Route.h"        // pyvrp::search::Route

#include <vector>

namespace pyvrp::search
{
/**
 * Solution(data: ProblemData)
 *
 * An alternative representation of a routing solution that is more amenable
 * to efficient modification. This is intended for use in the local search.
 *
 * This solution struct owns a vector of nodes, for the depots and clients. It
 * additionally owns a vector of (search) routes, which store non-owning
 * pointers into the nodes to model route visits. Modifying the solution via
 * search operators involves copying pointers, not whole nodes. That is very
 * efficient in practice.
 *
 * The solution does not protect its internal state---it is just a simple
 * wrapper around nodes and routes. Ensuring the solution remains valid is
 * up to the interacting code.
 */
struct Solution
{
    std::vector<Route::Node> nodes;  // size numLocations()
    std::vector<Route> routes;       // size numVehicles(), ordered by type

    Solution(ProblemData const &data);

    // Converts the given solution into our node-based representation.
    void load(ProblemData const &data, pyvrp::Solution const &solution);

    // Converts from our representation to a proper solution.
    pyvrp::Solution unload(ProblemData const &data) const;
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_SOLUTION_H
