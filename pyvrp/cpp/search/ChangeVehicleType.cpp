#include "ChangeVehicleType.h"

using pyvrp::search::ChangeVehicleType;

void ChangeVehicleType::operator()(PerturbationContext const &context)
{
    if (context.numPerturb == 0)
        return;

    auto const isEmpty = [](auto const &route) { return route.empty(); };
    size_t numChanged = 0;

    for (auto const idx : context.orderRoutes)
    {
        auto &routeU = context.routes[idx];
        if (routeU.empty() || routeU.numTrips() > 1)
            continue;

        for (auto const &[vehType, offset] : context.orderVehTypes)
        {
            if (routeU.vehicleType() == vehType)
                continue;  // skip routes of the same type

            auto const begin = context.routes.begin() + offset;
            auto const end = begin + data_.vehicleType(vehType).numAvailable;
            auto empty = std::find_if(begin, end, isEmpty);

            if (empty == end)
                continue;

            Route &routeV = *empty;
            op.apply(routeU[0], routeV[0]);
            routeU.update();
            routeV.update();

            numChanged += routeV.size();
            for (auto const *node : routeV)
                context.promising[node->client()] = true;

            break;
        }

        if (numChanged >= context.numPerturb)
            break;
    }
}

ChangeVehicleType::ChangeVehicleType(ProblemData const &data)
    : data_(data), op(data)
{
}

template <>
bool pyvrp::search::supports<ChangeVehicleType>(ProblemData const &data)
{
    // Only support ChangingVehicleType if there are multiple vehicle types
    // with different fixed costs.
    if (data.numVehicleTypes() == 1)
        return false;

    Cost fixedCost = data.vehicleType(0).fixedCost;

    for (size_t idx = 1; idx < data.numVehicleTypes(); ++idx)
        if (data.vehicleType(idx).fixedCost != fixedCost)
            return true;

    return false;
}
