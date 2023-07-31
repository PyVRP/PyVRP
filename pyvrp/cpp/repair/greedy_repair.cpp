#include "repair.h"

using pyvrp::DynamicBitset;
using pyvrp::ProblemData;
using pyvrp::Solution;

using Client = size_t;

pyvrp::Cost
insertCost(Client client, Solution::Route const &route, size_t position)
{
    // TODO
}

Solution pyvrp::repair::greedyRepair(std::vector<Solution::Route> routes,
                                     DynamicBitset const &toInsert,
                                     ProblemData const &data,
                                     CostEvaluator const &costEvaluator)
{
    for (Client client = 1; client <= data.numClients(); ++client)
    {
        if (!toInsert[client])
            continue;

        // TODO
    }

    return {data, routes};
}
