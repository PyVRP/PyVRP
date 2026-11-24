#ifndef PYVRP_MUTUALLYEXCLUSIVEGROUP_H
#define PYVRP_MUTUALLYEXCLUSIVEGROUP_H

#include "Activity.h"

#include <string>
#include <vector>

namespace pyvrp
{
/**
 * MutuallyExclusiveGroup(
 *    activities: list[Activity] = [],
 *    required: bool = True,
 *    *,
 *    name: str = "",
 * )
 *
 * A group of client and shipment pickup activities from which exactly one
 * must be served if the group is required, and at most one otherwise.
 *
 * Parameters
 * ----------
 * activities
 *     The client and pickup activities in the group.
 * required
 *     Whether serving an activity from this group is required.
 * name
 *    Free-form name field for this group. Default empty.
 *
 * Attributes
 * ----------
 * activities
 *     The client and pickup activities in the group.
 * required
 *     Whether serving an activity from this group is required.
 * name
 *    Free-form name field for this group.
 *
 * Raises
 * ------
 * ValueError
 *     When the activities contain duplicates, an activity is added to the
 *     group twice, or an activity is neither a client nor a pickup.
 */
class MutuallyExclusiveGroup
{
    std::vector<Activity> activities_;  // activities in this group

public:
    bool const required;  // is visiting the group required?
    char const *name;     // Group name (for reference)

    explicit MutuallyExclusiveGroup(std::vector<Activity> activities = {},
                                    bool required = true,
                                    std::string name = "");

    bool operator==(MutuallyExclusiveGroup const &other) const;

    MutuallyExclusiveGroup(MutuallyExclusiveGroup const &group);
    MutuallyExclusiveGroup(MutuallyExclusiveGroup &&group);

    MutuallyExclusiveGroup &operator=(MutuallyExclusiveGroup const &group)
        = delete;
    MutuallyExclusiveGroup &operator=(MutuallyExclusiveGroup &&group) = delete;

    ~MutuallyExclusiveGroup();

    bool empty() const;
    size_t size() const;

    std::vector<Activity>::const_iterator begin() const;
    std::vector<Activity>::const_iterator end() const;

    std::vector<Activity> const &activities() const;

    void addActivity(Activity activity);
};
}  // namespace pyvrp

#endif  // PYVRP_MUTUALLYEXCLUSIVEGROUP_H
