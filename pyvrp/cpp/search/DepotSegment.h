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
    ProblemData::Depot const &depot_;
    size_t const idx_;

public:
    DepotSegment(pyvrp::ProblemData const &data, size_t depot)
        : depot_(data.depot(depot)), idx_(depot)
    {
    }

    pyvrp::search::Route const *route() const { return nullptr; }

    SegmentProxy front() const
    {
        return {{Activity::ActivityType::DEPOT, idx_}, depot_.location};
    }

    SegmentProxy back() const
    {
        return {{Activity::ActivityType::DEPOT, idx_}, depot_.location};
    }

    size_t size() const { return 1; }

    bool startsAtReloadDepot() const { return true; }
    bool endsAtReloadDepot() const { return true; }

    pyvrp::Distance distance([[maybe_unused]] size_t profile) const
    {
        return 0;
    }

    pyvrp::DurationSegment duration([[maybe_unused]] size_t profile) const
    {
        return {depot_, 0};  // service is handled while evaluating proposal
    }

    pyvrp::LoadSegment load([[maybe_unused]] size_t dimension) const
    {
        return {};
    }
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_DEPOTSEGMENT_H
