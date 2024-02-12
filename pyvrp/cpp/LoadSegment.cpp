#include "LoadSegment.h"

using pyvrp::Load;
using pyvrp::LoadSegment;

Load LoadSegment::demand() const { return demand_; }

Load LoadSegment::supply() const { return supply_; }

Load LoadSegment::maxLoad() const { return maxLoad_; }

LoadSegment::LoadSegment(ProblemData::Client const &client)
    : demand_(client.demand),
      supply_(client.supply),
      maxLoad_(std::max(client.demand, client.supply))
{
}

LoadSegment::LoadSegment(Load demand, Load supply, Load maxLoad)
    : demand_(demand), supply_(supply), maxLoad_(maxLoad)
{
}
