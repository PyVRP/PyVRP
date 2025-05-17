#include "primitives.h"

namespace
{
/**
 * Simple wrapper class that implements the required evaluation interface for
 * a single client that might not currently be in the solution.
 */
class ClientSegment
{
    pyvrp::ProblemData const &data;
    size_t client;

public:
    ClientSegment(pyvrp::ProblemData const &data, size_t client)
        : data(data), client(client)
    {
    }

    size_t first() const { return client; }
    size_t last() const { return client; }

    pyvrp::DistanceSegment distance([[maybe_unused]] size_t profile) const
    {
        return {};
    }

    pyvrp::DurationSegment duration([[maybe_unused]] size_t profile) const
    {
        pyvrp::ProblemData::Client const &clientData = data.location(client);
        return {clientData};
    }

    pyvrp::LoadSegment load(size_t dimension) const
    {
        return {data.location(client), dimension};
    }
};
}  // namespace

pyvrp::Cost pyvrp::search::insertCost(Route::Node *U,
                                      Route::Node *V,
                                      ProblemData const &data,
                                      CostEvaluator const &costEvaluator)
{
    if (!V->route() || U->isDepot())
        return 0;

    auto *route = V->route();
    ProblemData::Client const &client = data.location(U->client());

    Cost deltaCost
        = Cost(route->empty()) * route->fixedVehicleCost() - client.prize;

    costEvaluator.deltaCost<true>(
        deltaCost,
        Route::Proposal(route->before(V->idx()),
                        ClientSegment(data, U->client()),
                        route->after(V->idx() + 1)));

    return deltaCost;
}

pyvrp::Cost pyvrp::search::removeCost(Route::Node *U,
                                      ProblemData const &data,
                                      CostEvaluator const &costEvaluator)
{
    if (!U->route() || U->isDepot())
        return 0;

    auto *route = U->route();
    ProblemData::Client const &client = data.location(U->client());

    Cost deltaCost
        = client.prize - Cost(route->size() == 1) * route->fixedVehicleCost();

    costEvaluator.deltaCost<true>(deltaCost,
                                  Route::Proposal(route->before(U->idx() - 1),
                                                  route->after(U->idx() + 1)));

    return deltaCost;
}

pyvrp::Cost pyvrp::search::inplaceCost(Route::Node *U,
                                       Route::Node *V,
                                       ProblemData const &data,
                                       CostEvaluator const &costEvaluator)
{
    if (U->route() || !V->route())
        return 0;

    auto const *route = V->route();
    ProblemData::Client const &uClient = data.location(U->client());
    ProblemData::Client const &vClient = data.location(V->client());

    Cost deltaCost = vClient.prize - uClient.prize;

    costEvaluator.deltaCost<true>(
        deltaCost,
        Route::Proposal(route->before(V->idx() - 1),
                        ClientSegment(data, U->client()),
                        route->after(V->idx() + 1)));

    return deltaCost;
}

void pyvrp::search::loadSolution(Solution const &solution,
                                 ProblemData const &data,
                                 std::vector<Route> &routes,
                                 std::vector<Route::Node> &nodes)
{
    // First empty all routes.
    for (auto &route : routes)
        route.clear();

    // Determine offsets for vehicle types.
    std::vector<size_t> vehicleOffset(data.numVehicleTypes(), 0);
    for (size_t vehType = 1; vehType < data.numVehicleTypes(); vehType++)
    {
        auto const prevAvail = data.vehicleType(vehType - 1).numAvailable;
        vehicleOffset[vehType] = vehicleOffset[vehType - 1] + prevAvail;
    }

    // Load routes from solution.
    for (auto const &solRoute : solution.routes())
    {
        // Set up a container of all node visits. This lets us insert all
        // nodes in one go, requiring no intermediate updates.
        std::vector<Route::Node *> visits;
        visits.reserve(solRoute.size());
        for (auto const client : solRoute)
            visits.push_back(&nodes[client]);

        // Determine index of next route of this type to load, where we rely
        // on solution to be valid to not exceed the number of vehicles per
        // vehicle type.
        auto const idx = vehicleOffset[solRoute.vehicleType()]++;
        routes[idx].insert(1, visits.begin(), visits.end());
        routes[idx].update();
    }
}

pyvrp::Solution pyvrp::search::exportSolution(std::vector<Route> const &routes,
                                              ProblemData const &data)
{
    std::vector<pyvrp::Route> solRoutes;
    solRoutes.reserve(data.numVehicles());

    for (auto const &route : routes)
    {
        if (route.empty())
            continue;

        std::vector<size_t> visits;
        visits.reserve(route.size());

        for (auto *node : route)
            visits.push_back(node->client());

        solRoutes.emplace_back(data, visits, route.vehicleType());
    }

    return {data, solRoutes};
}
