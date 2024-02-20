#include "LoadSegment.h"

using pyvrp::Load;
using pyvrp::LoadSegment;

Load LoadSegment::delivery() const { return delivery_; }

Load LoadSegment::pickup() const { return pickup_; }

LoadSegment::LoadSegment(ProblemData::Client const &client)
    : delivery_(client.delivery),
      pickup_(client.pickup),
      load_(std::max(client.delivery, client.pickup))
{
}
