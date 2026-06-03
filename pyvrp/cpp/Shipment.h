#ifndef PYVRP_SHIPMENT_H
#define PYVRP_SHIPMENT_H

#include "Measure.h"

#include <limits>

namespace pyvrp
{
/**
 * TODO
 */
struct Shipment
{
    /**
     * Shared data for a pickup or delivery step.
     */
    struct Step
    {
        size_t const location;
        Duration const twEarly;
        Duration const twLate;
        Duration const serviceDuration;

        Step(size_t location,
             Duration twEarly = 0,
             Duration twLate = std::numeric_limits<Duration>::max(),
             Duration serviceDuration = 0);

        Step(Step const &step) = default;
        Step(Step &&step) = default;

        bool operator==(Step const &other) const = default;

        Step &operator=(Step const &step) = delete;
        Step &operator=(Step &&step) = delete;
    };

    Step const pickup;
    Step const delivery;
    std::vector<Load> const amount;
    Cost const prize;
    bool const required;
    char const *name;

    // TODO constructors
};
}  // namespace pyvrp

#endif  // PYVRP_SHIPMENT_H
