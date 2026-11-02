#ifndef PYVRP_CHARGER_H
#define PYVRP_CHARGER_H

#include <cstddef>
#include <string>

namespace pyvrp
{
/**
 * Charger(
 *    location: int,
 *    *,
 *    name: str = "",
 * )
 *
 * Simple data object storing all charger data as (read-only) properties. A
 * vehicle with a battery fully recharges when it visits a charger, see also
 * :doc:`../setup/concepts`.
 *
 * Parameters
 * ----------
 * location
 *     Physical location of this charger.
 * name
 *     Free-form name field for this charger. Default empty.
 *
 * Attributes
 * ----------
 * location
 *     Physical location of this charger.
 * name
 *     Free-form name field for this charger.
 */
struct Charger
{
    size_t const location;
    char const *name;  // Charger name (for reference)

    Charger(size_t location, std::string name = "");

    bool operator==(Charger const &other) const;

    Charger(Charger const &charger);
    Charger(Charger &&charger);

    Charger &operator=(Charger const &charger) = delete;
    Charger &operator=(Charger &&charger) = delete;

    ~Charger();
};
}  // namespace pyvrp

#endif  // PYVRP_CHARGER_H
