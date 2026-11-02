#include "InsertCharger.h"
#include "ChargerSegment.h"
#include "constants.h"

#include <cassert>

using pyvrp::search::InsertCharger;

std::pair<pyvrp::Cost, bool>
InsertCharger::evaluate(Route::Node *U, CostEvaluator const &costEvaluator)
{
    assert(!U->isDepot());
    stats_.numEvaluations++;

    auto const *route = U->route();
    if (!route || route->excessCharge() == 0)
        return std::make_pair(0, false);

    move_ = {};
    auto const pos = U->pos();
    auto const duration = route->rechargeDuration();

    for (size_t charger = 0; charger != data.numChargers(); ++charger)
    {
        Cost before = 0;  // charger directly before U
        costEvaluator.deltaCost<true>(
            before,
            Route::Proposal(route->before(pos - 1),
                            ChargerSegment(data, charger, duration),
                            route->after(pos)));

        if (before < move_.cost)
            move_ = {before, charger, pos};

        Cost after = 0;  // charger directly after U
        costEvaluator.deltaCost<true>(
            after,
            Route::Proposal(route->before(pos),
                            ChargerSegment(data, charger, duration),
                            route->after(pos + 1)));

        if (after < move_.cost)
            move_ = {after, charger, pos + 1};
    }

    return std::make_pair(move_.cost, move_.cost < -TOL);
}

void InsertCharger::apply(Route::Node *U) const
{
    stats_.numApplications++;

    Route::Node charger = {Activity::ActivityType::CHARGER, move_.charger};
    U->route()->insert(move_.pos, &charger);
}

std::string InsertCharger::name() const { return "InsertCharger"; }

bool InsertCharger::supports(ProblemData const &data)
{
    // We need a charger, and at least one vehicle type that can use it.
    if (data.numChargers() == 0)
        return false;

    for (auto const &vehType : data.vehicleTypes())
        if (vehType.hasBattery())
            return true;

    return false;
}
