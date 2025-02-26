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
        return {data.location(client)};
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
