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

    pyvrp::DistanceSegment distance([[maybe_unused]] size_t profile) const
    {
        auto const &clientData = data.client(client);
        return pyvrp::DistanceSegment(clientData.location);
    }

    pyvrp::DurationSegment duration([[maybe_unused]] size_t profile) const
    {
        auto const &clientData = data.client(client);
        return pyvrp::DurationSegment(clientData);
    }

    pyvrp::LoadSegment load() const
    {
        auto const &clientData = data.client(client);
        return pyvrp::LoadSegment(clientData);
    }
};
}  // namespace

pyvrp::Cost pyvrp::search::insertCost(Route::Node *U,
                                      Route::Node *V,
                                      ProblemData const &data,
                                      CostEvaluator const &costEvaluator)
{
    if (!V->route() || !U->isClient())
        return 0;

    auto *route = V->route();
    auto const &client = data.client(U->client());

    Cost deltaCost
        = Cost(route->empty()) * route->fixedVehicleCost() - client.prize;

    costEvaluator.deltaCost<true>(
        deltaCost,
        route->proposal(route->before(V->idx()),
                        ClientSegment(data, U->client()),
                        route->after(V->idx() + 1)));

    return deltaCost;
}

pyvrp::Cost pyvrp::search::removeCost(Route::Node *U,
                                      ProblemData const &data,
                                      CostEvaluator const &costEvaluator)
{
    if (!U->route() || !U->isClient())
        return 0;

    auto *route = U->route();
    auto const &client = data.client(U->client());

    Cost deltaCost
        = client.prize - Cost(route->size() == 1) * route->fixedVehicleCost();

    costEvaluator.deltaCost<true>(deltaCost,
                                  route->proposal(route->before(U->idx() - 1),
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
    auto const &uClient = data.client(U->client());
    auto const &vClient = data.client(V->client());

    Cost deltaCost = vClient.prize - uClient.prize;

    costEvaluator.deltaCost<true>(
        deltaCost,
        route->proposal(route->before(V->idx() - 1),
                        ClientSegment(data, U->client()),
                        route->after(V->idx() + 1)));

    return deltaCost;
}
