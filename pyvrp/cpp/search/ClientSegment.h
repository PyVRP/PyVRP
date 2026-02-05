#ifndef PYVRP_SEARCH_CLIENTSEGMENT_H
#define PYVRP_SEARCH_CLIENTSEGMENT_H

#include "ProblemData.h"

#include "DurationSegment.h"
#include "LoadSegment.h"

namespace pyvrp::search
{
/**
 * Simple wrapper class that implements the required evaluation interface for
 * a single client that might not currently be in the solution.
 */
class ClientSegment
{
    pyvrp::ProblemData const &data;
    size_t client;

public:
    ClientSegment(pyvrp::ProblemData const &data, size_t client)
        : data(data), client(client)
    {
        assert(client >= data.numDepots());  // must be an actual client
    }

    pyvrp::search::Route const *route() const { return nullptr; }

    size_t first() const { return client; }
    size_t last() const { return client; }
    size_t size() const { return 1; }

    bool startsAtReloadDepot() const { return false; }
    bool endsAtReloadDepot() const { return false; }

    pyvrp::Distance distance([[maybe_unused]] size_t profile) const
    {
        return 0;
    }

    pyvrp::DurationSegment duration([[maybe_unused]] size_t profile) const
    {
        pyvrp::ProblemData::Client const &clientData = data.location(client);
        return {clientData};
    }

    pyvrp::LoadSegment load(size_t dimension) const
    {
        return {data.location(client), dimension};
    }
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_CLIENTSEGMENT_H
