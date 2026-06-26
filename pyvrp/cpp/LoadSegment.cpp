#include "LoadSegment.h"

#include <cassert>
#include <fstream>

using pyvrp::Activity;
using pyvrp::Load;
using pyvrp::LoadSegment;

Load LoadSegment::initial() const { return initial_; }

Load LoadSegment::delta() const { return delta_; }

Load LoadSegment::increase() const { return increase_; }

LoadSegment::LoadSegment(Client const &client, size_t dimension)
    : initial_(client.delivery[dimension]),
      delta_(client.pickup[dimension] - client.delivery[dimension]),
      increase_(std::max<Load>(0, delta_))
{
}

LoadSegment::LoadSegment(Shipment const &shipment,
                         Activity::ActivityType type,
                         size_t dimension)
    : delta_(type == Activity::ActivityType::PICKUP
                 ? shipment.amount[dimension]
                 : -shipment.amount[dimension]),
      increase_(std::max<Load>(0, delta_))
{
    assert(type == Activity::ActivityType::PICKUP
           || type == Activity::ActivityType::DELIVERY);
}

LoadSegment::LoadSegment(VehicleType const &vehicleType, size_t dimension)
    : initial_(vehicleType.initialLoad[dimension])
{
}

std::ostream &operator<<(std::ostream &out, LoadSegment const &segment)
{
    // Define 'capacity' as current load, so we can see only the cumulative
    // excess load when printing.
    auto const capacity = segment.load();

    // clang-format off
    return out << "initial=" << segment.initial()
               << ", delta=" << segment.delta()
               << ", increase=" << segment.increase()
               << ", load=" << segment.load()
               << ", excess_load=" << segment.excessLoad(capacity);
    // clang-format on
}
