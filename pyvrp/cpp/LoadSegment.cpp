#include "LoadSegment.h"

using pyvrp::Load;
using pyvrp::LoadSegment;

Load LoadSegment::delivery() const { return delivery_; }

Load LoadSegment::pickup() const { return pickup_; }

LoadSegment::LoadSegment(ProblemData::Client const &client, size_t dimension)
    : delivery_(client.delivery[dimension]),
      pickup_(client.pickup[dimension]),
      load_(std::max<Load>(delivery_, pickup_))
{
}

LoadSegment::LoadSegment(ProblemData::VehicleType const &vehicleType,
                         size_t dimension)
    : delivery_(0),
      pickup_(vehicleType.initialLoad[dimension]),
      load_(vehicleType.initialLoad[dimension])
{
}
