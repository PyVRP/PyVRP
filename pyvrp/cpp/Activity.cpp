#include "Activity.h"

#include <cassert>

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

Activity::Activity(ActivityType type, size_t idx) : type_(type), idx_(idx) {}

Activity::Activity(std::string const &description)
    : type_(char2type(description.empty() ? ' ' : description[0])),
      idx_(std::stol(description.empty() ? 0 : description.substr(1)))
{
    assert(description.size() >= 2);  // sanity check; type/idx do validation
}

std::ostream &operator<<(std::ostream &out, pyvrp::Activity const &activity)
{
    return out << type2char(activity.type()) << activity.idx();
}
