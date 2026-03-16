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
 *     The index of the activity corresponding to the activity type. For
 *     example, if this is a client visit and ``idx`` is 0, then this activity
 *     visits the first client.
 */
class Activity
{
public:
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

private:
    ActivityType type_;
    size_t idx_;

public:
    Activity(ActivityType type, size_t idx);

    Activity(std::string const &description);

    bool operator==(Activity const &other) const = default;

    /**
     * The type of activity.
     */
    inline ActivityType type() const;

    /**
     *  The index of the activity corresponding to the activity type.
     */
    inline size_t idx() const;

    /**
     * Returns whether this activity concerns a client visit.
     */
    inline bool isClient() const;

    /**
     * Returns whether this activity concerns a depot visit.
     */
    inline bool isDepot() const;
};

Activity::ActivityType Activity::type() const { return type_; }

size_t Activity::idx() const { return idx_; }

bool Activity::isClient() const { return type_ == ActivityType::CLIENT; }

bool Activity::isDepot() const { return type_ == ActivityType::DEPOT; }
}  // namespace pyvrp

std::ostream &operator<<(std::ostream &out, pyvrp::Activity const &activity);

#endif  // PYVRP_ACTIVITY_H
