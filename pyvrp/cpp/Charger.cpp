#include "Charger.h"

#include <cstring>

using pyvrp::Charger;

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

Charger::Charger(size_t location, std::string name)
    : location(location), name(duplicate(name.data()))
{
}

Charger::Charger(Charger const &charger)
    : location(charger.location), name(duplicate(charger.name))
{
}

Charger::Charger(Charger &&charger)
    : location(charger.location), name(charger.name)  // we can steal
{
    charger.name = nullptr;  // stolen
}

Charger::~Charger() { delete[] name; }

bool Charger::operator==(Charger const &other) const
{
    return location == other.location && std::strcmp(name, other.name) == 0;
}
