#include "LoadSegment.h"

#include <fstream>

using pyvrp::Load;
using pyvrp::LoadSegment;

Load LoadSegment::delivery() const { return delivery_; }

Load LoadSegment::pickup() const { return pickup_; }

Load LoadSegment::load() const { return load_; }

LoadSegment::LoadSegment(ProblemData::Client const &client, size_t dimension)
    : delivery_(client.delivery[dimension]),
      pickup_(client.pickup[dimension]),
      load_(std::max<Load>(delivery_, pickup_))
{
}

LoadSegment::LoadSegment(ProblemData::VehicleType const &vehicleType,
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
               << ", excess_load=" << segment.excessLoad(capacity);
    // clang-format on
}
