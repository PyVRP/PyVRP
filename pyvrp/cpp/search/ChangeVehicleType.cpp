#include "ChangeVehicleType.h"

void pyvrp::search::ChangeVehicleType::operator()(
    PerturbationContext const &context)
{
    if (numPerturb_ == 0)
        return;

    auto const &orderVehTypes = context.orderVehTypes;
    auto const &orderRoutes = context.orderRoutes;
    auto &routes = context.routes;

    auto const isEmpty = [](auto const &route) { return route.empty(); };
    size_t numChanged = 0;

    for (auto const idx : orderRoutes)
    {
        auto &routeU = routes[idx];
        if (routeU.empty())
            continue;

        for (auto const &[vehType, offset] : orderVehTypes)
        {
            if (routeU.vehicleType() == vehType)
                continue;  // skip routes of the same type

            auto const begin = routes.begin() + offset;
            auto const end = begin + data_.vehicleType(vehType).numAvailable;

            // Find an empty route if available, otherwise the first route.
            auto rV = std::find_if(begin, end, isEmpty);
            if (rV == end)
                rV = begin;

            Route &routeV = *rV;

            if (routeU.numTrips() > 1 || routeV.numTrips() > 1)
                continue;

            op.apply(routeU[0], routeV[0]);
            routeU.update();
            routeV.update();
            numChanged += routeU.size() + routeV.size();
        }

        if (numChanged >= numPerturb_)
            break;
    }
}

pyvrp::search::ChangeVehicleType::ChangeVehicleType(ProblemData const &data,
                                                    size_t const numPerturb)
    : data_(data), numPerturb_(numPerturb), op(data)
{
}
