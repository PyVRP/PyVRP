#ifndef PYVRP_LOCATION_H
#define PYVRP_LOCATION_H

#include "Measure.h"

#include <string>

namespace pyvrp
{
/**
 * Location(
 *     x: float,
 *     y: float,
 *     *,
 *     name: str = "",
 * )
 *
 * Represents a physical location.
 *
 * Parameters
 * ----------
 * x
 *     Horizontal coordinate of this location. This can for example be a
 *     longitude value.
 * y
 *     Vertical coordinate of this location. This can for example be a
 *     latitude value.
 * name
 *     Free-form name field for this location. Default empty.
 *
 * Attributes
 * ----------
 * x
 *     Horizontal location coordinate.
 * y
 *     Vertical location coordinate.
 * name
 *     Free-form name field for this location.
 */
struct Location
{
    Coordinate const x;
    Coordinate const y;
    char const *name;

    Location(Coordinate x, Coordinate y, std::string name = "");

    bool operator==(Location const &other) const;

    Location(Location const &location);
    Location(Location &&location);

    Location &operator=(Location const &location) = delete;
    Location &operator=(Location &&location) = delete;

    ~Location();
};
}  // namespace pyvrp

#endif  // PYVRP_LOCATION_H
