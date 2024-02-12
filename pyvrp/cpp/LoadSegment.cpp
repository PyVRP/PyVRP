#include "LoadSegment.h"

using pyvrp::Load;
using pyvrp::LoadSegment;

Load LoadSegment::demand() const { return demand_; }

Load LoadSegment::supply() const { return supply_; }

LoadSegment::LoadSegment(ProblemData::Client const &client)
    : demand_(client.demand),
      supply_(client.supply),
      load_(std::max(client.demand, client.supply))
{
}

LoadSegment::LoadSegment(Load demand, Load supply, Load load)
    : demand_(demand), supply_(supply), load_(load)
{
}
