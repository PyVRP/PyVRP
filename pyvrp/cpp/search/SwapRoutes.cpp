#include "SwapRoutes.h"

#include <vector>

using pyvrp::Cost;
using pyvrp::search::Route;
using pyvrp::search::SwapRoutes;
using TWS = pyvrp::TimeWindowSegment;

Cost SwapRoutes::evaluate(Route *U,
                          Route *V,
                          CostEvaluator const &costEvaluator)
{
    if (U->vehicleType() == V->vehicleType() || U->empty() || V->empty())
        return 0;

    Cost deltaCost = 0;

    // Changes in load capacity violations.
    deltaCost += costEvaluator.loadPenalty(U->load(), V->capacity());
    deltaCost -= costEvaluator.loadPenalty(U->load(), U->capacity());

    deltaCost += costEvaluator.loadPenalty(V->load(), U->capacity());
    deltaCost -= costEvaluator.loadPenalty(V->load(), V->capacity());

    // Changes in time warp.
    deltaCost -= costEvaluator.twPenalty(U->timeWarp());
    deltaCost -= costEvaluator.twPenalty(V->timeWarp());

    auto const uTWS = TWS::merge(data.durationMatrix(),
                                 V->tws(0),
                                 U->twsBetween(1, U->size()),
                                 V->tws(V->size() + 1));

    deltaCost += costEvaluator.twPenalty(uTWS.timeWarp(V->maxDuration()));

    auto const vTWS = TWS::merge(data.durationMatrix(),
                                 U->tws(0),
                                 V->twsBetween(1, V->size()),
                                 U->tws(U->size() + 1));

    deltaCost += costEvaluator.twPenalty(vTWS.timeWarp(U->maxDuration()));

    // TODO handle the case of depot differences (multiple depots). There is
    // some evaluation code for this in issue #188.
    return deltaCost;
}

void SwapRoutes::apply(Route *U, Route *V) const
{
    std::vector<Route::Node *> uNodes = {U->begin(), U->end()};
    std::vector<Route::Node *> vNodes = {V->begin(), V->end()};

    // We are swapping the routes completely, so we can just clear both.
    U->clear();
    V->clear();

    // Reinsert the nodes from route U in route V.
    for (auto *node : uNodes)
        V->push_back(node);

    // Reinsert the nodes from route V in route U.
    for (auto *node : vNodes)
        U->push_back(node);
}

SwapRoutes::SwapRoutes(ProblemData const &data)
    : LocalSearchOperator<Route>(data)
{
}
