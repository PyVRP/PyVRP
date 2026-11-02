#ifndef PYVRP_SEARCH_CHARGERSEGMENT_H
#define PYVRP_SEARCH_CHARGERSEGMENT_H

#include "Activity.h"
#include "BatterySegment.h"
#include "DurationSegment.h"
#include "LoadSegment.h"
#include "ProblemData.h"

namespace pyvrp::search
{
/**
 * Simple wrapper class that implements the required evaluation interface for
 * a single charger that might not currently be in the solution.
 */
class ChargerSegment
{
    Charger const &charger_;
    size_t const idx_;
    Duration const rechargeDuration_;

public:
    ChargerSegment(pyvrp::ProblemData const &data,
                   size_t charger,
                   Duration rechargeDuration)
        : charger_(data.charger(charger)),
          idx_(charger),
          rechargeDuration_(rechargeDuration)
    {
    }

    pyvrp::search::Route const *route() const { return nullptr; }

    SegmentProxy front() const
    {
        return {{Activity::ActivityType::CHARGER, idx_}, charger_.location};
    }

    SegmentProxy back() const
    {
        return {{Activity::ActivityType::CHARGER, idx_}, charger_.location};
    }

    size_t size() const { return 1; }
    size_t numClients() const { return 0; }
    size_t numPickups() const { return 0; }

    bool startsAtReloadDepot() const { return false; }
    bool endsAtReloadDepot() const { return false; }

    pyvrp::Distance distance([[maybe_unused]] size_t profile) const
    {
        return 0;
    }

    pyvrp::DurationSegment duration([[maybe_unused]] size_t profile) const
    {
        return {rechargeDuration_};
    }

    pyvrp::LoadSegment load([[maybe_unused]] size_t dimension) const
    {
        return {};
    }

    pyvrp::BatterySegment
    battery([[maybe_unused]] size_t profile,
            [[maybe_unused]] pyvrp::Distance capacity) const
    {
        return {0, 0, 0, true};
    }
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_CHARGERSEGMENT_H
