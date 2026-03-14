#ifndef PYVRP_SEARCH_CLIENTSEGMENT_H
#define PYVRP_SEARCH_CLIENTSEGMENT_H

#include "Activity.h"
#include "DurationSegment.h"
#include "LoadSegment.h"
#include "ProblemData.h"

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
        assert(client < data.numClients());  // must be an actual client
    }

    pyvrp::search::Route const *route() const { return nullptr; }

    SegmentProxy front() const
    {
        return {{Activity::ActivityType::CLIENT, client},
                data.client(client).location};
    }

    SegmentProxy back() const
    {
        return {{Activity::ActivityType::CLIENT, client},
                data.client(client).location};
    }

    size_t size() const { return 1; }

    bool startsAtReloadDepot() const { return false; }
    bool endsAtReloadDepot() const { return false; }

    pyvrp::Distance distance([[maybe_unused]] size_t profile) const
    {
        return 0;
    }

    pyvrp::DurationSegment duration([[maybe_unused]] size_t profile) const
    {
        auto const &clientData = data.client(client);
        return {clientData};
    }

    pyvrp::LoadSegment load(size_t dimension) const
    {
        return {data.client(client), dimension};
    }
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_CLIENTSEGMENT_H
