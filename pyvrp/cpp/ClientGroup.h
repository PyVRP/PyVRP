#ifndef PYVRP_CLIENTGROUP_H
#define PYVRP_CLIENTGROUP_H

#include "Measure.h"

#include <string>
#include <vector>

namespace pyvrp
{
/**
 * ClientGroup(
 *    clients: list[int] = [],
 *    required: bool = True,
 *    *,
 *    name: str = "",
 * )
 *
 * A client group that imposes additional restrictions on visits to clients
 * in the group.
 *
 * .. note::
 *
 *    Only mutually exclusive client groups are supported for now.
 *
 * Parameters
 * ----------
 * clients
 *     The clients in the group.
 * required
 *     Whether visiting this client group is required.
 * name
 *    Free-form name field for this client group. Default empty.
 *
 * Attributes
 * ----------
 * clients
 *     The clients in the group.
 * required
 *     Whether visiting this client group is required.
 * mutually_exclusive
 *     When ``True``, exactly one of the clients in this group must be
 *     visited if the group is required, and at most one if the group is
 *     not required.
 * name
 *    Free-form name field for this client group.
 *
 * Raises
 * ------
 * ValueError
 *     When the given clients contain duplicates, or when a client is added
 *     to the group twice.
 */
class ClientGroup
{
    std::vector<size_t> clients_;  // clients in this group

public:
    bool const required;                  // is visiting the group required?
    bool const mutuallyExclusive = true;  // at most one visit in group?
    char const *name;                     // Group name (for reference)

    explicit ClientGroup(std::vector<size_t> clients = {},
                         bool required = true,
                         std::string name = "");

    bool operator==(ClientGroup const &other) const;

    ClientGroup(ClientGroup const &group);
    ClientGroup(ClientGroup &&group);

    ClientGroup &operator=(ClientGroup const &group) = delete;
    ClientGroup &operator=(ClientGroup &&group) = delete;

    ~ClientGroup();

    bool empty() const;
    size_t size() const;

    std::vector<size_t>::const_iterator begin() const;
    std::vector<size_t>::const_iterator end() const;

    std::vector<size_t> const &clients() const;

    void addClient(size_t client);
};
}  // namespace pyvrp

#endif  // PYVRP_CLIENTGROUP_H
