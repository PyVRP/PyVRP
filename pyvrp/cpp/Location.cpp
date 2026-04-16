#include "Location.h"

#include <cstring>

using pyvrp::Location;

Location::Location(Coordinate x, Coordinate y, std::string name)
    : x(x), y(y), name(std::strdup(name.data()))
{
}

Location::Location(Location const &location)
    : x(location.x), y(location.y), name(std::strdup(location.name))
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
