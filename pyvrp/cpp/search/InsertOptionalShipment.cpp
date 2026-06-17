#include "InsertOptionalShipment.h"

using pyvrp::search::InsertOptionalShipment;

std::pair<pyvrp::Cost, bool> InsertOptionalShipment::evaluate(
    [[maybe_unused]] Route::Node *U,
    [[maybe_unused]] Route::Node *V,
    [[maybe_unused]] CostEvaluator const &costEvaluator)
{
    stats_.numEvaluations++;

    // TODO

    return std::make_pair(0, false);
}

void InsertOptionalShipment::apply([[maybe_unused]] Route::Node *U,
                                   [[maybe_unused]] Route::Node *V) const
{
    stats_.numApplications++;

    // TODO
}

template <>
bool pyvrp::search::supports<InsertOptionalShipment>(ProblemData const &data)
{
    for (auto const &shipment : data.shipments())  // need at least one
        if (!shipment.required)                    // optional shipment
            return true;

    return false;
}
