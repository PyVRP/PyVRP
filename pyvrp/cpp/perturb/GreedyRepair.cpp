#include "GreedyRepair.h"
#include "search/primitives.h"

void pyvrp::perturb::GreedyRepair::operator()(
    std::vector<search::Route::Node> &nodes,
    std::vector<search::Route> &routes,
    CostEvaluator const &costEvaluator,
    std::vector<std::vector<size_t>> const &neighbours,
    RandomNumberGenerator &rng)
{
    std::vector<size_t> unplanned;
    unplanned.reserve(data_.numClients());

    for (size_t idx = data_.numDepots(); idx != data_.numLocations(); ++idx)
    {
        auto *U = &nodes[idx];
        if (U->route())
            continue;

        ProblemData::Client const &uData = data_.location(idx);
        if (!uData.required && (rng.randint(100) < skipOptionalProbability))
            continue;

        unplanned.push_back(idx);
    }

    std::shuffle(unplanned.begin(), unplanned.end(), rng);

    for (auto const client : unplanned)
    {
        auto *U = &nodes[client];

        search::Route::Node *UAfter = routes[0][0];
        Cost bestCost = insertCost(U, UAfter, data_, costEvaluator);

        for (auto const vClient : neighbours[client])
        {
            auto *V = &nodes[vClient];

            if (!V->route())
                continue;

            auto const cost = insertCost(U, V, data_, costEvaluator);
            if (cost < bestCost)
            {
                bestCost = cost;
                UAfter = V;
            }
        }

        // Try inserting in a random empty route. We first store the indices
        // of an empty route for each vehicle type, if available.
        auto begin = routes.begin();
        std::vector<size_t> emptyIdcs;
        for (size_t vehType = 0; vehType != data_.numVehicleTypes(); vehType++)
        {
            auto const end = begin + data_.vehicleType(vehType).numAvailable;
            auto const pred = [](auto const &route) { return route.empty(); };
            auto empty = std::find_if(begin, end, pred);
            begin = end;

            if (empty != end)
                emptyIdcs.push_back(std::distance(routes.begin(), empty));
        }

        if (!emptyIdcs.empty())
        {
            // This helps because empty vehicle moves incur fixed cost,
            // and a purely greedy approach over-prioritises vehicles with low
            // fixed costs but possibly high variable costs.
            auto &empty = routes[emptyIdcs[rng.randint(emptyIdcs.size())]];
            auto const cost = insertCost(U, empty[0], data_, costEvaluator);
            if (cost < bestCost)
            {
                bestCost = cost;
                UAfter = empty[0];
            }
        }

        assert(UAfter && UAfter->route());
        UAfter->route()->insert(UAfter->idx() + 1, U);
        UAfter->route()->update();
    }
}

pyvrp::perturb::GreedyRepair::GreedyRepair(ProblemData const &data,
                                           size_t skipOptionalProbability)
    : data_(data), skipOptionalProbability(skipOptionalProbability)
{
}
