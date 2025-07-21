#include "OptionalInsert.h"
#include "primitives.h"

void pyvrp::search::OptionalInsert::operator()(
    PerturbationContext const &context)
{
    if (numPerturb_ == 0 || data_.numClients() == 0)
        return;

    auto const &costEvaluator = context.costEvaluator;
    size_t numInserts = 0;

    for (auto const uClient : context.orderNodes)
    {
        ProblemData::Client clientData = data_.location(uClient);
        if (clientData.required)
            continue;

        auto *U = &context.nodes[uClient];
        if (U->route())
            continue;

        Route::Node *UAfter = nullptr;
        Cost bestCost = std::numeric_limits<Cost>::max();

        for (auto const vClient : context.neighbours[uClient])
        {
            auto *V = &context.nodes[vClient];

            if (!V->route())
                continue;

            auto const cost = insertCost(U, V, data_, costEvaluator);
            if (cost < bestCost)
            {
                bestCost = cost;
                UAfter = V;
            }
        }

        // Also consider an empty route, if available.
        auto const &[vehType, offset] = context.orderVehTypes.front();
        auto const begin = context.routes.begin() + offset;
        auto const end = begin + data_.vehicleType(vehType).numAvailable;
        auto const isEmpty = [](auto const &route) { return route.empty(); };
        auto empty = std::find_if(begin, end, isEmpty);

        if (empty != end)
        {
            auto const cost = insertCost(U, (*empty)[0], data_, costEvaluator);
            if (cost < bestCost)
            {
                bestCost = cost;
                UAfter = (*empty)[0];
            }
        }

        if (!UAfter)
            continue;

        UAfter->route()->insert(UAfter->idx() + 1, U);
        UAfter->route()->update();

        if (++numInserts >= numPerturb_)
            break;
    }
}

pyvrp::search::OptionalInsert::OptionalInsert(ProblemData const &data,
                                              size_t const numPerturb)
    : data_(data), numPerturb_(numPerturb)
{
}
