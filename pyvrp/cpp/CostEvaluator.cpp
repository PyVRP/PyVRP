#include "CostEvaluator.h"

#include <stdexcept>

using pyvrp::CostEvaluator;

CostEvaluator::CostEvaluator(std::vector<double> loadPenalties,
                             double twPenalty,
                             double distPenalty,
                             double distDevPenalty)
    : loadPenalties_(std::move(loadPenalties)),
      twPenalty_(twPenalty),
      distPenalty_(distPenalty),
      distDevPenalty_(distDevPenalty)
{
    for (auto const penalty : loadPenalties_)
        if (penalty < 0)
            throw std::invalid_argument("load_penalties must be >= 0.");

    if (twPenalty_ < 0)
        throw std::invalid_argument("tw_penalty must be >= 0.");

    if (distPenalty_ < 0)
        throw std::invalid_argument("dist_penalty must be >= 0.");

    if (distDevPenalty_ < 0)
        throw std::invalid_argument("dist_dev_penalty must be >= 0.");
}
