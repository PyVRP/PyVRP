#include "StringRemoval.h"
#include "primitives.h"

#include <algorithm>
#include <vector>

void pyvrp::search::StringRemoval::operator()(
    PerturbationContext const &context)
{
    if (data_.numClients() == 0)
        return;

    auto const maxPerRoute = 10;

    std::vector<size_t> nodes = {context.orderNodes.front()};
    for (auto const neighbour : context.neighbours[nodes.front()])
        nodes.push_back(neighbour);

    std::vector<size_t> removed;

    size_t numDestroyed = 0;
    for (auto const idx : nodes)
    {
        auto *U = &context.nodes[idx];
        if (!U->route())
            continue;

        auto *route = U->route();
        size_t routeDestroyed = 0;  // Count per route

        while (!U->isDepot() && routeDestroyed < maxPerRoute)
        {
            context.promising[U->client()] = true;
            context.promising[p(U)->client()] = true;
            context.promising[n(U)->client()] = true;
            removed.push_back(U->client());

            auto next = n(U);
            route->remove(U->idx());
            route->update();
            U = next;

            routeDestroyed++;
            if (++numDestroyed == context.numPerturb)
                break;
        }

        if (numDestroyed == context.numPerturb)
            break;
    }

    // Insert removed clients back.
    for (auto const client : removed)
    {
        auto *U = &context.nodes[client];
        Cost bestCost = std::numeric_limits<Cost>::max();
        Route::Node *UAfter = nullptr;

        for (auto const vClient : context.neighbours[client])
        {
            auto *V = &context.nodes[vClient];
            if (!V->route())
                continue;

            auto const cost = insertCost(U, V, data_, context.costEvaluator);
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
            auto const cost
                = insertCost(U, (*empty)[0], data_, context.costEvaluator);
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

        context.promising[U->client()] = true;
        context.promising[p(U)->client()] = true;
        context.promising[n(U)->client()] = true;
    }
}

pyvrp::search::StringRemoval::StringRemoval(ProblemData const &data)
    : data_(data)
{
}
