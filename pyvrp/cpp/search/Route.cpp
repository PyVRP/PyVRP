#define _USE_MATH_DEFINES  // needed to get M_PI etc. on Windows builds
// TODO use std::numbers::pi instead of M_PI when C++20 is supported by CIBW

#include "Route.h"

#include <cmath>
#include <ostream>

using TWS = TimeWindowSegment;

Route::Route(ProblemData const &data, size_t const idx, size_t const vehType)
    : data(data), vehicleType_(vehType), idx(idx)
{
}

void Route::setupNodes()
{
    nodes.clear();
    auto *node = depot;

    do
    {
        node = n(node);
        nodes.push_back(node);
    } while (!node->isDepot());
}

void Route::setupSector()
{
    if (empty())  // Note: sector has no meaning for empty routes, don't use
        return;

    auto const &depotData = data.client(0);
    auto const &clientData = data.client(n(depot)->client);

    auto const diffX = static_cast<double>(clientData.x - depotData.x);
    auto const diffY = static_cast<double>(clientData.y - depotData.y);
    auto const angle = CircleSector::positive_mod(
        static_cast<int>(32768. * atan2(diffY, diffX) / M_PI));

    sector.initialize(angle);

    for (auto it = nodes.begin(); it != nodes.end() - 1; ++it)
    {
        auto const *node = *it;
        assert(!node->isDepot());

        auto const &clientData = data.client(node->client);

        auto const diffX = static_cast<double>(clientData.x - depotData.x);
        auto const diffY = static_cast<double>(clientData.y - depotData.y);
        auto const angle = CircleSector::positive_mod(
            static_cast<int>(32768. * atan2(diffY, diffX) / M_PI));

        sector.extend(angle);
    }
}

void Route::setupRouteTimeWindows()
{
    auto *node = nodes.back();

    do  // forward time window segments
    {
        auto *prev = p(node);
        prev->twAfter
            = TWS::merge(data.durationMatrix(), prev->tw, node->twAfter);
        node = prev;
    } while (!node->isDepot());
}

size_t Route::vehicleType() const { return vehicleType_; }

bool Route::overlapsWith(Route const &other, int const tolerance) const
{
    return CircleSector::overlap(sector, other.sector, tolerance);
}

void Route::update()
{
    auto const oldNodes = nodes;
    setupNodes();

    Load load = 0;
    Distance distance = 0;
    Distance reverseDistance = 0;
    bool foundChange = false;

    for (size_t pos = 0; pos != nodes.size(); ++pos)
    {
        auto *node = nodes[pos];

        if (!foundChange && (pos >= oldNodes.size() || node != oldNodes[pos]))
        {
            foundChange = true;

            if (pos > 0)  // change at pos, so everything before is the same
            {             // and we can re-use cumulative calculations
                load = nodes[pos - 1]->cumulatedLoad;
                distance = nodes[pos - 1]->cumulatedDistance;
                reverseDistance = nodes[pos - 1]->cumulatedReversalDistance;
            }
        }

        if (!foundChange)
            continue;

        load += data.client(node->client).demand;
        distance += data.dist(p(node)->client, node->client);

        reverseDistance += data.dist(node->client, p(node)->client);
        reverseDistance -= data.dist(p(node)->client, node->client);

        node->position = pos + 1;
        node->cumulatedLoad = load;
        node->cumulatedDistance = distance;
        node->cumulatedReversalDistance = reverseDistance;
        node->twBefore
            = TWS::merge(data.durationMatrix(), p(node)->twBefore, node->tw);
    }

    setupSector();
    setupRouteTimeWindows();

    load_ = nodes.back()->cumulatedLoad;
    isLoadFeasible_ = load_ <= capacity();

    timeWarp_ = nodes.back()->twBefore.totalTimeWarp();
    isTimeWarpFeasible_ = timeWarp_ == 0;
}

std::ostream &operator<<(std::ostream &out, Route const &route)
{
    out << "Route #" << route.idx + 1 << ":";  // route number
    for (auto *node = n(route.depot); !node->isDepot(); node = n(node))
        out << ' ' << node->client;  // client index
    out << '\n';

    return out;
}
