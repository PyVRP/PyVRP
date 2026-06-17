#include "ReplaceOptionalShipment.h"

#include <cassert>

using pyvrp::search::ReplaceOptionalShipment;

std::pair<pyvrp::Cost, bool> ReplaceOptionalShipment::evaluate(
    [[maybe_unused]] Route::Node *U,
    [[maybe_unused]] Route::Node *V,
    [[maybe_unused]] CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    // TODO

    return std::make_pair(0, false);
}

void ReplaceOptionalShipment::apply([[maybe_unused]] Route::Node *U,
                                    [[maybe_unused]] Route::Node *V) const
{
    stats_.numApplications++;

    // TODO
}

template <>
bool pyvrp::search::supports<ReplaceOptionalShipment>(ProblemData const &data)
{
    for (auto const &shipment : data.shipments())  // need at least one
        if (!shipment.required)                    // optional shipment
            return true;

    return false;
}
