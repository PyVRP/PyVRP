#ifndef PYVRP_MUTUALLYEXCLUSIVEGROUP_H
#define PYVRP_MUTUALLYEXCLUSIVEGROUP_H

#include "Measure.h"

#include <string>
#include <vector>

namespace pyvrp
{
/**
 * MutuallyExclusiveGroup(
 *    clients: list[int] = [],
 *    required: bool = True,
 *    *,
 *    name: str = "",
 * )
 *
 * A group of clients from which exactly one must be visited if the group is
 * required, and at most one otherwise.
 *
 * Parameters
 * ----------
 * clients
 *     The clients in the group.
 * required
 *     Whether visiting a client from this group is required.
 * name
 *    Free-form name field for this group. Default empty.
 *
 * Attributes
 * ----------
 * clients
 *     The clients in the group.
 * required
 *     Whether visiting a client from this group is required.
 * mutually_exclusive
 *     When ``True``, exactly one of the clients in this group must be
 *     visited if the group is required, and at most one if the group is
 *     not required.
 * name
 *    Free-form name field for this group.
 *
 * Raises
 * ------
 * ValueError
 *     When the given clients contain duplicates, or when a client is added
 *     to the group twice.
 */
class MutuallyExclusiveGroup
{
    std::vector<size_t> clients_;  // clients in this group

public:
    bool const required;  // is visiting the group required?
    char const *name;     // Group name (for reference)

    explicit MutuallyExclusiveGroup(std::vector<size_t> clients = {},
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

    std::vector<size_t>::const_iterator begin() const;
    std::vector<size_t>::const_iterator end() const;

    std::vector<size_t> const &clients() const;

    void addClient(size_t client);
};
}  // namespace pyvrp

#endif  // PYVRP_MUTUALLYEXCLUSIVEGROUP_H
