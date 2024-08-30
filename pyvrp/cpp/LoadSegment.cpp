#include "LoadSegment.h"

using pyvrp::Load;
using pyvrp::LoadSegment;

Load LoadSegment::delivery() const { return delivery_; }

Load LoadSegment::pickup() const { return pickup_; }

LoadSegment::LoadSegment(ProblemData::Client const &client)
    : delivery_(client.delivery[0]),
      pickup_(client.pickup[0]),
      load_(std::max(client.delivery[0], client.pickup[0]))
{
}
