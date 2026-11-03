#include "MutuallyExclusiveGroup.h"

#include <cstring>

using pyvrp::MutuallyExclusiveGroup;

namespace
{
// Small local helper for what is essentially strdup() from the C23 standard,
// which my compiler does not (yet) have. See here for the actual recipe:
// https://stackoverflow.com/a/252802/4316405 (modified to use new instead of
// malloc). We do all this so we can use C-style strings, rather than C++'s
// std::string, which are much larger objects.
static char *duplicate(char const *src)
{
    char *dst = new char[std::strlen(src) + 1];  // space for src + null
    std::strcpy(dst, src);
    return dst;
}
}  // namespace

MutuallyExclusiveGroup::MutuallyExclusiveGroup(std::vector<size_t> clients,
                                               bool required,
                                               std::string name)
    : required(required), name(duplicate(name.data()))
{
    for (auto const client : clients)
        addClient(client);
}

MutuallyExclusiveGroup::MutuallyExclusiveGroup(
    MutuallyExclusiveGroup const &group)
    : clients_(group.clients_),
      required(group.required),
      name(duplicate(group.name))
{
}

MutuallyExclusiveGroup::MutuallyExclusiveGroup(MutuallyExclusiveGroup &&group)
    : clients_(std::move(group.clients_)),
      required(group.required),
      name(group.name)  // we can steal
{
    group.name = nullptr;  // stolen
}

bool MutuallyExclusiveGroup::operator==(
    MutuallyExclusiveGroup const &other) const
{
    // clang-format off
    return clients_ == other.clients_
        && required == other.required
        && mutuallyExclusive == other.mutuallyExclusive
        && std::strcmp(name, other.name) == 0;
    // clang-format on
}

MutuallyExclusiveGroup::~MutuallyExclusiveGroup() { delete[] name; }

bool MutuallyExclusiveGroup::empty() const { return clients_.empty(); }

size_t MutuallyExclusiveGroup::size() const { return clients_.size(); }

std::vector<size_t>::const_iterator MutuallyExclusiveGroup::begin() const
{
    return clients_.begin();
}

std::vector<size_t>::const_iterator MutuallyExclusiveGroup::end() const
{
    return clients_.end();
}

std::vector<size_t> const &MutuallyExclusiveGroup::clients() const
{
    return clients_;
}

void MutuallyExclusiveGroup::addClient(size_t client)
{
    if (std::find(clients_.begin(), clients_.end(), client) != clients_.end())
        throw std::invalid_argument("Client already in group.");

    clients_.push_back(client);
}
