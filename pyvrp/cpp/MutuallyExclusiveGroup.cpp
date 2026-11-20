#include "MutuallyExclusiveGroup.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

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

MutuallyExclusiveGroup::MutuallyExclusiveGroup(std::vector<Activity> activities,
                                               bool required,
                                               std::string name)
    : required(required), name(duplicate(name.data()))
{
    for (auto const activity : activities)
        addActivity(activity);
}

MutuallyExclusiveGroup::MutuallyExclusiveGroup(
    MutuallyExclusiveGroup const &group)
    : activities_(group.activities_),
      required(group.required),
      name(duplicate(group.name))
{
}

MutuallyExclusiveGroup::MutuallyExclusiveGroup(MutuallyExclusiveGroup &&group)
    : activities_(std::move(group.activities_)),
      required(group.required),
      name(group.name)  // we can steal
{
    group.name = nullptr;  // stolen
}

bool MutuallyExclusiveGroup::operator==(
    MutuallyExclusiveGroup const &other) const
{
    // clang-format off
    return activities_ == other.activities_
        && required == other.required
        && std::strcmp(name, other.name) == 0;
    // clang-format on
}

MutuallyExclusiveGroup::~MutuallyExclusiveGroup() { delete[] name; }

bool MutuallyExclusiveGroup::empty() const { return activities_.empty(); }

size_t MutuallyExclusiveGroup::size() const { return activities_.size(); }

std::vector<pyvrp::Activity>::const_iterator
MutuallyExclusiveGroup::begin() const
{
    return activities_.begin();
}

std::vector<pyvrp::Activity>::const_iterator MutuallyExclusiveGroup::end() const
{
    return activities_.end();
}

std::vector<pyvrp::Activity> const &MutuallyExclusiveGroup::activities() const
{
    return activities_;
}

void MutuallyExclusiveGroup::addActivity(Activity activity)
{
    if (!activity.isClient() && !activity.isPickup())
        throw std::invalid_argument(
            "Group activity must be a client or pickup.");

    if (std::find(activities_.begin(), activities_.end(), activity)
        != activities_.end())
        throw std::invalid_argument("Activity already in group.");

    activities_.push_back(activity);
}
