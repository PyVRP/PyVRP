#ifndef PYVRP_SEARCH_PICKUPSEGMENT_H
#define PYVRP_SEARCH_PICKUPSEGMENT_H

#include "Activity.h"
#include "DurationSegment.h"
#include "LoadSegment.h"
#include "ProblemData.h"

namespace pyvrp::search
{
/**
 * Simple wrapper class that implements the required evaluation interface for
 * a shipment pickup step.
 */
class PickupSegment
{
    Shipment const &shipment_;
    size_t const idx_;

public:
    PickupSegment(pyvrp::ProblemData const &data, size_t shipment)
        : shipment_(data.shipment(shipment)), idx_(shipment)
    {
    }

    pyvrp::search::Route const *route() const { return nullptr; }

    SegmentProxy front() const
    {
        return {{Activity::ActivityType::DELIVERY, idx_},
                shipment_.delivery.location};
    }

    SegmentProxy back() const
    {
        return {{Activity::ActivityType::DELIVERY, idx_},
                shipment_.delivery.location};
    }

    size_t size() const { return 1; }
    size_t numClients() const { return 0; }

    bool startsAtReloadDepot() const { return false; }
    bool endsAtReloadDepot() const { return false; }

    pyvrp::Distance distance([[maybe_unused]] size_t profile) const
    {
        return 0;
    }

    pyvrp::DurationSegment duration([[maybe_unused]] size_t profile) const
    {
        return {shipment_.delivery};
    }

    pyvrp::LoadSegment load(size_t dimension) const
    {
        return {shipment_, Activity::ActivityType::DELIVERY, dimension};
    }
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_PICKUPSEGMENT_H
