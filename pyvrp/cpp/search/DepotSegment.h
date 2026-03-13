#ifndef PYVRP_SEARCH_DEPOTSEGMENT_H
#define PYVRP_SEARCH_DEPOTSEGMENT_H

#include "Activity.h"
#include "DurationSegment.h"
#include "LoadSegment.h"
#include "ProblemData.h"

namespace pyvrp::search
{
/**
 * Simple wrapper class that implements the required evaluation interface for
 * a single (reload) depot.
 */
class DepotSegment
{
    pyvrp::ProblemData const &data_;
    size_t depot_;

public:
    DepotSegment(pyvrp::ProblemData const &data, size_t depot)
        : data_(data), depot_(depot)
    {
        assert(depot < data.numDepots());  // must be an actual depot
    }

    pyvrp::search::Route const *route() const { return nullptr; }

    Activity first() const { return {Activity::ActivityType::DEPOT, depot_}; }
    Activity last() const { return {Activity::ActivityType::DEPOT, depot_}; }
    size_t size() const { return 1; }

    bool startsAtReloadDepot() const { return true; }
    bool endsAtReloadDepot() const { return true; }

    pyvrp::Distance distance([[maybe_unused]] size_t profile) const
    {
        return 0;
    }

    pyvrp::DurationSegment duration([[maybe_unused]] size_t profile) const
    {
        auto const &depot = data_.depot(depot_);
        return {depot, 0};  // service is handled while evaluating proposal
    }

    pyvrp::LoadSegment load([[maybe_unused]] size_t dimension) const
    {
        return {};
    }
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_DEPOTSEGMENT_H
