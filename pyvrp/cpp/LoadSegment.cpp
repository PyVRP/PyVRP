#include "LoadSegment.h"

using pyvrp::Load;
using pyvrp::LoadSegment;

LoadSegment::LoadSegment(ProblemData::Client const &client, size_t dimension)
    : delivery_(client.delivery[dimension]),
      pickup_(client.pickup[dimension]),
      load_(std::max<Load>(delivery_, pickup_))
{
}
