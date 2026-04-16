#include "Location.h"

#include <cstring>

using pyvrp::Location;

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

Location::Location(Coordinate x, Coordinate y, std::string name)
    : x(x), y(y), name(duplicate(name.data()))
{
}

Location::Location(Location const &location)
    : x(location.x), y(location.y), name(duplicate(location.name))
{
}

Location::Location(Location &&location)
    : x(location.x), y(location.y), name(location.name)  // we can steal
{
    location.name = nullptr;  // stolen
}

bool Location::operator==(Location const &other) const
{
    return x == other.x && y == other.y && std::strcmp(name, other.name) == 0;
}

Location::~Location() { delete[] name; }
