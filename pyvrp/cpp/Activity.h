#ifndef PYVRP_ACTIVITY_H
#define PYVRP_ACTIVITY_H

#include <cstddef>

namespace pyvrp
{
/**
 * Activity(type: ActivityType, idx: int)
 *
 * TODO
 *
 * Parameters
 * ----------
 * type
 *     TODO
 * idx
 *     TODO
 */
struct Activity
{
    /**
     * Activity types.
     */
    enum class ActivityType
    {
        CLIENT,
        DEPOT,
    };

    ActivityType const type;
    size_t const idx;

    Activity(ActivityType type, size_t idx);

    bool operator==(Activity const &other) const = default;

    /**
     * Returns whether this activity concerns a client visit.
     */
    inline bool isClient() const;

    /**
     * Returns whether this activity concerns a depot visit.
     */
    inline bool isDepot() const;
};
}  // namespace pyvrp

bool pyvrp::Activity::isClient() const { return type == ActivityType::CLIENT; }

bool pyvrp::Activity::isDepot() const { return type == ActivityType::DEPOT; }

#endif  // PYVRP_ACTIVITY_H
