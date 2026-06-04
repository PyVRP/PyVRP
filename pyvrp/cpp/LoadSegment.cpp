#include "LoadSegment.h"

#include <cassert>
#include <fstream>

using pyvrp::Activity;
using pyvrp::Load;
using pyvrp::LoadSegment;

Load LoadSegment::delivery() const { return delivery_; }

Load LoadSegment::pickup() const { return pickup_; }

Load LoadSegment::load() const { return QMax_ + load_; }

Load LoadSegment::QSum() const { return QSum_; }

Load LoadSegment::QMax() const { return QMax_; }

LoadSegment::LoadSegment(Client const &client, size_t dimension)
    : delivery_(client.delivery[dimension]),
      pickup_(client.pickup[dimension]),
      load_(std::max<Load>(delivery_, pickup_))
{
}

LoadSegment::LoadSegment(Shipment const &shipment,
                         Activity::ActivityType type,
                         size_t dimension)
    : QSum_(type == Activity::ActivityType::PICKUP
                ? shipment.amount[dimension]
                : -shipment.amount[dimension]),
      QMax_(std::max<Load>(QSum_, 0))
{
    assert(type == Activity::ActivityType::PICKUP
           || type == Activity::ActivityType::DELIVERY);
}

LoadSegment::LoadSegment(VehicleType const &vehicleType,
                         size_t dimension)
    :  // Initial load is always a pickup quantity: it's already on the vehicle,
       // and needs to be dropped off at a (reload) depot.
      pickup_(vehicleType.initialLoad[dimension]),
      load_(vehicleType.initialLoad[dimension])
{
}

std::ostream &operator<<(std::ostream &out, LoadSegment const &segment)
{
    // Define 'capacity' as current load, so we can see only the cumulative
    // excess load when printing.
    auto const capacity = segment.load();

    // clang-format off
    return out << "delivery=" << segment.delivery() 
               << ", pickup=" << segment.pickup()
               << ", load=" << segment.load()
               << ", QSum=" << segment.QSum()
               << ", QMax=" << segment.QMax()
               << ", excess_load=" << segment.excessLoad(capacity);
    // clang-format on
}
