#ifndef PYVRP_ACTIVITY_H
#define PYVRP_ACTIVITY_H

#include <cstdint>
#include <cstdlib>

namespace pyvrp
{
/**
 * ActivityType()
 *
 * Describes the type of an :class:`~pyvrp._pyvrp.Activity`.
 */
enum class ActivityType : uint8_t
{
    Depot,
    Client,
};

/**
 * Activity(type: ActivityType, index: int)
 *
 * A single activity performed during a route. An activity is either a depot
 * visit or a client visit.
 *
 * Attributes
 * ----------
 * type : ActivityType
 *     The type of this activity.
 * index : int
 *     Location index in :class:`~pyvrp._pyvrp.ProblemData` of this activity.
 */
struct Activity
{
    ActivityType type;
    size_t index;

    bool operator==(Activity const &) const = default;
};
}  // namespace pyvrp

#endif  // PYVRP_ACTIVITY_H
