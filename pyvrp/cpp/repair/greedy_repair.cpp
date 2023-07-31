#include "repair.h"

using pyvrp::DynamicBitset;
using pyvrp::ProblemData;
using pyvrp::Solution;

Solution pyvrp::repair::greedyRepair(std::vector<Solution::Route> const &routes,
                                     DynamicBitset const &toInsert,
                                     ProblemData const &data,
                                     CostEvaluator const &costEvaluator)
{
    // TODO
    return {data, routes};
}
