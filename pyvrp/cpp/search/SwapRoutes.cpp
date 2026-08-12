#include "SwapRoutes.h"

#include <algorithm>
#include <cassert>

using pyvrp::search::SwapRoutes;

namespace
{
bool representsRoute(pyvrp::search::Route::Node *node)
{
    auto const *route = node->route();
    if (!route)
        return false;

    if (route->empty())
        return node->isStartDepot();

    return (*route)[1] == node;
}
}  // namespace

SwapRoutes::SwapRoutes(ProblemData const &data)
    : BinaryOperator(data), swapTails_(data)
{
}

std::pair<pyvrp::Cost, bool> SwapRoutes::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    if (!representsRoute(U) || !representsRoute(V))
    {
        stats_.numEvaluations++;
        return std::make_pair(0, false);
    }

    auto *uRoute = U->route();
    auto *vRoute = V->route();
    assert(uRoute && vRoute);

    if (uRoute == vRoute || uRoute->vehicleType() == vRoute->vehicleType()
        || (uRoute->empty() && vRoute->empty()))
    {
        stats_.numEvaluations++;
        return std::make_pair(0, false);
    }

    auto *uDepot = (*uRoute)[0];
    auto *vDepot = (*vRoute)[0];
    if (!uRoute->empty() && !vRoute->empty() && uRoute > vRoute)
        std::swap(uDepot, vDepot);

    auto const result = swapTails_.evaluate(uDepot, vDepot, costEvaluator);
    stats_.numEvaluations++;
    return result;
}

void SwapRoutes::apply(Route::Node *U, Route::Node *V) const
{
    assert(representsRoute(U) && representsRoute(V));
    assert(U->route() != V->route());

    auto *uDepot = (*U->route())[0];
    auto *vDepot = (*V->route())[0];
    swapTails_.apply(uDepot, vDepot);
    stats_.numApplications++;
}

void SwapRoutes::init(Solution &solution)
{
    BinaryOperator::init(solution);
    swapTails_.init(solution);
}

std::string SwapRoutes::name() const { return "SwapRoutes"; }

template <> bool pyvrp::search::supports<SwapRoutes>(ProblemData const &data)
{
    return data.numVehicleTypes() >= 2;
}
