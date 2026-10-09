#ifndef PYVRP_SHIPMENT_H
#define PYVRP_SHIPMENT_H

#include "Measure.h"

#include <limits>

namespace pyvrp
{
/**
 * Shipment(
 *     pickup_location: int,
 *     delivery_location: int,
 *     pickup_tw_early: int = 0,
 *     pickup_tw_late: int = np.iinfo(np.int64).max,
 *     pickup_service_duration: int = 0,
 *     delivery_tw_early: int = 0,
 *     delivery_tw_late: int = np.iinfo(np.int64).max,
 *     delivery_service_duration: int = 0,
 *     amount: list[int] = [],
 *     prize: int = 0,
 *     required: bool = True,
 *     *,
 *     name: str = "",
 * )
 *
 * Simple data object storing all shipment data as properties. See also
 * :doc:`../setup/concepts` for further information about these properties.
 *
 * Parameters
 * ----------
 * pickup_location
 *     Physical location where the shipment must be picked up.
 * delivery_location
 *     Physical location where the shipment must be delivered.
 * pickup_tw_early
 *     Earliest time at which the shipment may be picked up. Default 0.
 * pickup_tw_late
 *     Latest time at which the shipment may be picked up. Unconstrained if not
 *     provided.
 * pickup_service_duration
 *     Amount of time a vehicle takes to load the shipment at the pickup
 *     location. Default 0.
 * delivery_tw_early
 *     Earliest time at which the shipment may be delivered. Default 0.
 * delivery_tw_late
 *     Latest time at which the shipment may be delivered. Unconstrained if not
 *     provided.
 * delivery_service_duration
 *     Amount of time a vehicle takes to unload the shipment at the delivery
 *     location. Default 0.
 * amount
 *     The amount of goods that are transported in this shipment.
 * prize
 *     Prize collected by serving this shipment. Default 0. If this shipment
 *     is not required, the prize needs to be sufficiently large to offset
 *     any travel cost before this shipment will be in a solution.
 * required
 *     Whether this shipment is required in a feasible solution. Default True.
 * name
 *     Free-form name field for this shipment. Default empty.
 *
 * Attributes
 * ----------
 * pickup
 *     Attribute grouping data related to the pickup step of this shipment.
 * delivery
 *     Attribute grouping data related to the delivery step of this shipment.
 * amount
 *     Amount of goods that are transported in this shipment.
 * prize
 *     Prize collected by serving this shipment.
 * required
 *     Whether serving this shipment is required.
 * name
 *     Free-form name field for this shipment.
 */
struct Shipment
{
    /**
     * ShipmentStep(
     *     location: int,
     *     tw_early: int = 0,
     *     tw_late: int = np.iinfo(np.int64).max,
     *     service_duration: int = 0,
     * )
     *
     * Data for a pickup or delivery step.
     *
     * Parameters
     * ----------
     * location
     *     Physical location of this step.
     * tw_early
     *     Earliest time at which service at this step may begin. Default 0.
     * tw_late
     *     Latest time at which service at this step may begin. Unconstrained
     *     if not provided.
     * service_duration
     *     Service duration at this step. Default 0.
     *
     * Attributes
     * ----------
     * location
     *     Physical step location.
     * tw_early
     *     Earliest service time.
     * tw_late
     *     Latest service time.
     * service_duration
     *     Duration of the service.
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

        bool operator==(Step const &other) const = default;

        Step(Step const &step) = default;
        Step(Step &&step) = default;

        Step &operator=(Step const &step) = delete;
        Step &operator=(Step &&step) = delete;
    };

    Step const pickup;
    Step const delivery;
    std::vector<Load> const amount;
    Cost const prize;
    bool const required;
    char const *name;

    Shipment(size_t pickupLocation,
             size_t deliveryLocation,
             Duration pickupTwEarly = 0,
             Duration pickupTwLate = std::numeric_limits<Duration>::max(),
             Duration pickupServiceDuration = 0,
             Duration deliveryTwEarly = 0,
             Duration deliveryTwLate = std::numeric_limits<Duration>::max(),
             Duration deliveryServiceDuration = 0,
             std::vector<Load> amount = {},
             Cost prize = 0,
             bool required = true,
             std::string name = "");

    Shipment(Step pickup,  // constructor used for serialisation
             Step delivery,
             std::vector<Load> amount,
             Cost prize,
             bool required,
             std::string name);

    bool operator==(Shipment const &other) const;

    Shipment(Shipment const &shipment);
    Shipment(Shipment &&shipment);

    Shipment &operator=(Shipment const &shipment) = delete;
    Shipment &operator=(Shipment &&shipment) = delete;

    ~Shipment();
};
}  // namespace pyvrp

#endif  // PYVRP_SHIPMENT_H
