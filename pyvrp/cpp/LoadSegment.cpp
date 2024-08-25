#include "LoadSegment.h"

using pyvrp::Load;
using pyvrp::LoadSegment;

std::vector<Load> LoadSegment::delivery() const { return delivery_; }

std::vector<Load> LoadSegment::pickup() const { return pickup_; }

LoadSegment::LoadSegment(ProblemData::Client const &client)
    : delivery_(client.delivery), pickup_(client.pickup)
{
    for (size_t idx = 0; idx != delivery_.size(); ++idx)
        load_.push_back(std::max(delivery_[idx], pickup_[idx]));
}
