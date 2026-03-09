#ifndef PYVRP_ACTIVITY_H
#define PYVRP_ACTIVITY_H

#include <cstddef>
#include <ostream>
#include <string>

namespace pyvrp
{
/**
 * Activity(type: ActivityType, idx: int)
 *
 * A route activity.
 *
 * Parameters
 * ----------
 * type
 *     The type of activity, for example a depot or client visit.
 * idx
 *     The unique index of the activity corresponding to the activity type. For
 *     example, if this is a client visit and ``idx`` is 0, then this activity
 *     visits the first client.
 */
struct Activity
{
    /**
     * Enum of activity types.
     *
     * Attributes
     * ----------
     * DEPOT
     *     A depot visit.
     * CLIENT
     *     A client visit.
     */
    enum class ActivityType
    {
        DEPOT = 0,
        CLIENT = 1,
    };

    ActivityType const type;
    size_t const idx;

    Activity(ActivityType type, size_t idx);

    Activity(std::string const &description);

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

std::ostream &operator<<(std::ostream &out, pyvrp::Activity const &activity);

#endif  // PYVRP_ACTIVITY_H
