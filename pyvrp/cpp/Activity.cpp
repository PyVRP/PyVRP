#include "Activity.h"

using pyvrp::Activity;

namespace
{
Activity::ActivityType char2type(char type)
{
    switch (type)
    {
    case 'D':
        return Activity::ActivityType::DEPOT;
    case 'C':
        return Activity::ActivityType::CLIENT;
        break;
    default:
        throw std::invalid_argument("Activity type not understood.");
    }
}

char type2char(Activity::ActivityType type)
{
    switch (type)
    {
    case Activity::ActivityType::DEPOT:
        return 'D';
    case Activity::ActivityType::CLIENT:
        return 'C';
    default:
        throw std::invalid_argument("Activity type not understood.");
    }
}
}  // namespace

Activity::Activity(ActivityType type, size_t idx) : type(type), idx(idx) {}

Activity::Activity(std::string const &description)
    : type(char2type(description.empty() ? ' ' : description[0])),
      idx(std::stol(description.empty() ? 0 : description.substr(1)))
{
    if (description.size() < 2)
        throw std::invalid_argument(
            "Activity description must include at least a type and an index.");
}

std::ostream &operator<<(std::ostream &out, pyvrp::Activity const &activity)
{
    return out << type2char(activity.type) << activity.idx;
}
