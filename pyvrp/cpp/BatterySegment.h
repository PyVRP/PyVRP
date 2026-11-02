#ifndef PYVRP_BATTERYSEGMENT_H
#define PYVRP_BATTERYSEGMENT_H

#include "Measure.h"
#include "VehicleType.h"

#include <algorithm>

namespace pyvrp
{
/**
 * BatterySegment(
 *     head: int = 0,
 *     tail: int = 0,
 *     excess_charge: int = 0,
 *     charges: bool = False,
 * )
 *
 * Creates a new battery segment. These segments can be efficiently
 * concatenated, and track statistics about battery charge violations resulting
 * from visiting chargers in the concatenated order. Vehicles drain one unit of
 * charge per unit of distance, so all fields are in distance units.
 *
 * Parameters
 * ----------
 * head
 *     Charge drained before the first charger on this segment. If there is no
 *     such charger, this is the total charge drained on this segment.
 * tail
 *     Charge drained after the last charger on this segment. If there is no
 *     such charger, this is the total charge drained on this segment.
 * excess_charge
 *     Cumulative excess charge between the chargers on this segment.
 * charges
 *     Whether this segment visits a charger.
 */
class BatterySegment
{
    Distance head_ = 0;     // drained before the first charger
    Distance tail_ = 0;     // drained after the last charger
    Distance excess_ = 0;   // excess charge between the chargers
    bool charges_ = false;  // does this segment visit a charger?

public:
    /**
     * Merges the two given segments, connected by an edge of the given
     * distance. The given capacity is the capacity of the vehicle's battery.
     */
    [[nodiscard]] static inline BatterySegment
    merge(Distance edgeDistance,
          BatterySegment const &first,
          BatterySegment const &second,
          Distance capacity);

    /**
     * Charge drained before the first charger on this segment. If there is no
     * such charger, this is the total charge drained on this segment.
     */
    [[nodiscard]] inline Distance head() const;

    /**
     * Charge drained after the last charger on this segment. If there is no
     * such charger, this is the total charge drained on this segment.
     */
    [[nodiscard]] inline Distance tail() const;

    /**
     * Whether this segment visits a charger.
     */
    [[nodiscard]] inline bool charges() const;

    /**
     * Returns the charge drained on this segment that the battery could not
     * supply.
     *
     * Parameters
     * ----------
     * capacity
     *     Capacity of the vehicle's battery.
     */
    [[nodiscard]] inline Distance excessCharge(Distance capacity) const;

    BatterySegment() = default;  // default is all zero

    // Construct from the initial charge of the given vehicle type. A battery
    // that is not full starts as if the difference has already been drained.
    inline explicit BatterySegment(VehicleType const &vehicleType);

    // Construct from raw data.
    inline BatterySegment(Distance head,
                          Distance tail,
                          Distance excessCharge = 0,
                          bool charges = false);

    // Move or copy construct from the other battery segment.
    inline BatterySegment(BatterySegment const &) = default;
    inline BatterySegment(BatterySegment &&) = default;

    // Move or copy assign form the other battery segment.
    inline BatterySegment &operator=(BatterySegment const &) = default;
    inline BatterySegment &operator=(BatterySegment &&) = default;
};

BatterySegment BatterySegment::merge(Distance edgeDistance,
                                     BatterySegment const &first,
                                     BatterySegment const &second,
                                     Distance capacity)
{
    // Charge drained between the last charger of the first segment and the
    // first charger of the second segment.
    auto const drained = first.tail_ + edgeDistance + second.head_;

    if (first.charges_ && second.charges_)
    {
        auto const excess = std::max<Distance>(drained - capacity, 0);
        return {first.head_,
                second.tail_,
                first.excess_ + second.excess_ + excess,
                true};
    }

    if (first.charges_)
        return {first.head_, drained, first.excess_, true};

    if (second.charges_)
        return {drained, second.tail_, second.excess_, true};

    return {drained, drained};
}

Distance BatterySegment::head() const { return head_; }

Distance BatterySegment::tail() const { return tail_; }

bool BatterySegment::charges() const { return charges_; }

Distance BatterySegment::excessCharge(Distance capacity) const
{
    auto const head = std::max<Distance>(head_ - capacity, 0);
    auto const tail = std::max<Distance>(tail_ - capacity, 0);

    // Without a charger, head and tail are one and the same stretch.
    return charges_ ? excess_ + head + tail : excess_ + head;
}

BatterySegment::BatterySegment(VehicleType const &vehicleType)
    : head_(vehicleType.batteryCapacity - vehicleType.initialCharge),
      tail_(head_)
{
}

BatterySegment::BatterySegment(Distance head,
                               Distance tail,
                               Distance excessCharge,
                               bool charges)
    : head_(head), tail_(tail), excess_(excessCharge), charges_(charges)
{
}
}  // namespace pyvrp

#endif  // PYVRP_BATTERYSEGMENT_H
