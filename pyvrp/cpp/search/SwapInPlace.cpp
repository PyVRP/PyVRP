#include "SwapInPlace.h"
#include "primitives.h"

using pyvrp::Cost;
using pyvrp::search::SwapInPlace;

Cost SwapInPlace::evaluate(Route::Node *U,
                           Route::Node *V,
                           CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    ProblemData::Client const &uClient = data.location(U->client());
    ProblemData::Client const &vClient = data.location(V->client());

    if (uClient.required)
        return 0;

    if (uClient.group)
    {
        auto const uGroup = data.group(uClient.group.value());
        if (uGroup.required && vClient.group
            && uGroup != data.group(vClient.group.value()))
            return 0;  // cannot swap required group client with non-required
    }

    return inplaceCost(V, U, data, costEvaluator);
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
