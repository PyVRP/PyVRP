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
 *     elevation: int = 0,
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
 * elevation
 *     Elevation of this location in meters. Default 0.
 * name
 *     Free-form name field for this location. Default empty.
 *
 * Attributes
 * ----------
 * x
 *     Horizontal location coordinate.
 * y
 *     Vertical location coordinate.
 * elevation
 *     Elevation of this location in meters.
 * name
 *     Free-form name field for this location.
 */
struct Location
{
    Coordinate const x;
    Coordinate const y;
    Distance const elevation;
    char const *name;

    Location(Coordinate x,
             Coordinate y,
             Distance elevation = 0,
             std::string name = "");

    bool operator==(Location const &other) const;

    Location(Location const &location);
    Location(Location &&location);

    Location &operator=(Location const &location) = delete;
    Location &operator=(Location &&location) = delete;

    ~Location();
};
}  // namespace pyvrp

#endif  // PYVRP_LOCATION_H
