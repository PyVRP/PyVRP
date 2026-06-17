#include "RemoveOptionalShipment.h"

#include <cassert>

using pyvrp::search::RemoveOptionalShipment;

std::pair<pyvrp::Cost, bool> RemoveOptionalShipment::evaluate(
    [[maybe_unused]] Route::Node *U,
    [[maybe_unused]] CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    // TODO

    return std::make_pair(0, false);
}

void RemoveOptionalShipment::apply([[maybe_unused]] Route::Node *U) const
{
    stats_.numApplications++;

    // TODO
}

template <>
bool pyvrp::search::supports<RemoveOptionalShipment>(ProblemData const &data)
{
    for (auto const &shipment : data.shipments())  // need at least one
        if (!shipment.required)                    // optional shipment
            return true;

    return false;
}
