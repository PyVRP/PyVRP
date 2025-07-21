#include "NeighbourRemoval.h"
#include "primitives.h"

using pyvrp::search::NeighbourRemoval;

void NeighbourRemoval::operator()(PerturbationContext const &context)
{
    if (context.numPerturb == 0 || data_.numClients() == 0)
        return;

    auto const client = context.orderNodes[0];  // random client
    std::vector<size_t> removed;

    for (auto const idx : context.neighbours[client])
    {
        auto *U = &context.nodes[idx];
        if (!U->route())
            continue;

        context.promising[U->client()] = true;
        context.promising[p(U)->client()] = true;
        context.promising[n(U)->client()] = true;

        auto *nU = n(U);
        auto *pU = p(U);

        removed.push_back(U->client());
        auto *route = U->route();
        route->remove(U->idx());
        route->update();

        if (!nU->isDepot())
        {
            context.promising[nU->client()] = true;
            context.promising[p(nU)->client()] = true;
            context.promising[n(nU)->client()] = true;

            removed.push_back(nU->client());
            auto *route = nU->route();
            route->remove(nU->idx());
            route->update();
        }

        if (!pU->isDepot())
        {
            context.promising[pU->client()] = true;
            context.promising[p(pU)->client()] = true;
            context.promising[n(pU)->client()] = true;

            removed.push_back(pU->client());
            auto *route = pU->route();
            route->remove(pU->idx());
            route->update();
        }

        if (removed.size() >= context.numPerturb)
            break;
    }

    // // Insert removed clients back.
    // for (auto const client : removed)
    // {
    //     auto *U = &context.nodes[client];
    //     Cost bestCost = std::numeric_limits<Cost>::max();
    //     Route::Node *UAfter = nullptr;

    //     for (auto const vClient : context.neighbours[client])
    //     {
    //         auto *V = &context.nodes[vClient];
    //         if (!V->route())
    //             continue;

    //         auto const cost = insertCost(U, V, data_, context.costEvaluator);
    //         if (cost < bestCost)
    //         {
    //             bestCost = cost;
    //             UAfter = V;
    //         }
    //     }

    //     // Also consider an empty route, if available.
    //     auto const &[vehType, offset] = context.orderVehTypes.front();
    //     auto const begin = context.routes.begin() + offset;
    //     auto const end = begin + data_.vehicleType(vehType).numAvailable;
    //     auto const isEmpty = [](auto const &route) { return route.empty(); };
    //     auto empty = std::find_if(begin, end, isEmpty);

    //     if (empty != end)
    //     {
    //         auto const cost
    //             = insertCost(U, (*empty)[0], data_, context.costEvaluator);
    //         if (cost < bestCost)
    //         {
    //             bestCost = cost;
    //             UAfter = (*empty)[0];
    //         }
    //     }

    //     if (!UAfter)
    //         continue;

    //     UAfter->route()->insert(UAfter->idx() + 1, U);
    //     UAfter->route()->update();

    //     context.promising[U->client()] = true;
    //     context.promising[p(U)->client()] = true;
    //     context.promising[n(U)->client()] = true;
    // }
}

NeighbourRemoval::NeighbourRemoval(ProblemData const &data) : data_(data) {}
