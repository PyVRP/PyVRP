#include "Depot.h"

#include <cstring>

using pyvrp::Depot;

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

Depot::Depot(size_t location,
             Duration twEarly,
             Duration twLate,
             Duration serviceDuration,
             std::string name)
    : location(location),
      serviceDuration(serviceDuration),
      twEarly(twEarly),
      twLate(twLate),
      name(duplicate(name.data()))
{
    if (serviceDuration < 0)
        throw std::invalid_argument("service_duration must be >= 0.");

    if (twEarly > twLate)
        throw std::invalid_argument("tw_early must be <= tw_late.");

    if (twEarly < 0)
        throw std::invalid_argument("tw_early must be >= 0.");
}

Depot::Depot(Depot const &depot)
    : location(depot.location),
      serviceDuration(depot.serviceDuration),
      twEarly(depot.twEarly),
      twLate(depot.twLate),
      name(duplicate(depot.name))
{
}

Depot::Depot(Depot &&depot)
    : location(depot.location),
      serviceDuration(depot.serviceDuration),
      twEarly(depot.twEarly),
      twLate(depot.twLate),
      name(depot.name)  // we can steal
{
    depot.name = nullptr;  // stolen
}

Depot::~Depot() { delete[] name; }

bool Depot::operator==(Depot const &other) const
{
    // clang-format off
    return location == other.location
        && twEarly == other.twEarly
        && twLate == other.twLate
        && serviceDuration == other.serviceDuration
        && std::strcmp(name, other.name) == 0;
    // clang-format on
}
