#include "SwapInPlace.h"

using pyvrp::Cost;
using pyvrp::search::SwapInPlace;

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
        assert(client >= data.numDepots());  // must be an actual client
    }

    pyvrp::search::Route const *route() const { return nullptr; }

    size_t first() const { return client; }
    size_t last() const { return client; }
    size_t size() const { return 1; }

    bool startsAtReloadDepot() const { return false; }
    bool endsAtReloadDepot() const { return false; }

    pyvrp::Distance distance([[maybe_unused]] size_t profile) const
    {
        return 0;
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

Cost SwapInPlace::evaluate(Route::Node *U,
                           Route::Node *V,
                           CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    if (!U->route() || V->route())
        return 0;

    ProblemData::Client const &uClient = data.location(U->client());
    ProblemData::Client const &vClient = data.location(V->client());

    if (uClient.required)
        return 0;

    if (uClient.group)
    {
        auto const uGroup = data.group(uClient.group.value());
        if (uGroup.required && uClient.group != vClient.group)
            return 0;  // cannot swap required group client with non-required
    }

    auto const *route = U->route();
    Cost deltaCost = uClient.prize - vClient.prize;

    costEvaluator.deltaCost(deltaCost,
                            Route::Proposal(route->before(U->idx() - 1),
                                            ClientSegment(data, V->client()),
                                            route->after(U->idx() + 1)));

    return deltaCost;
}

void SwapInPlace::apply(Route::Node *U, Route::Node *V) const
{
    stats_.numApplications++;

    auto const *route = U->route();
    route->swap(U, V);
}

template <> bool pyvrp::search::supports<SwapInPlace>(ProblemData const &data)
{
    // This operator only works if the problem has at least one optional client.
    return std::any_of(data.clients().begin(),
                       data.clients().end(),
                       [](ProblemData::Client const &client)
                       { return !client.required && !client.group; });
}
