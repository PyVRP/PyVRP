#define _USE_MATH_DEFINES  // needed to get M_PI etc. on Windows builds
// TODO use std::numbers::pi instead of M_PI when C++20 is supported by CIBW

#include "Route.h"

#include <cmath>
#include <ostream>

using TWS = TimeWindowSegment;

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
    if (empty())
    {
        angleCenter = 1.e30;
        return;
    }

    auto const depotData = data->client(0);
    auto const clientData = data->client(n(depot)->client);
    auto const angle = CircleSector::positive_mod(static_cast<int>(
        32768. * atan2(clientData.y - depotData.y, clientData.x - depotData.x)
        / M_PI));

    sector.initialize(angle);

    int cumulatedX = 0;
    int cumulatedY = 0;

    for (auto it = nodes.begin(); it != nodes.end() - 1; ++it)
    {
        auto const *node = *it;
        assert(!node->isDepot());

        cumulatedX += data->client(node->client).x;
        cumulatedY += data->client(node->client).y;

        auto const clientData = data->client(node->client);
        auto const angle = CircleSector::positive_mod(static_cast<int>(
            32768.
            * atan2(clientData.y - depotData.y, clientData.x - depotData.x)
            / M_PI));

        sector.extend(angle);
    }

    // This computes a pseudo-angle that sorts roughly equivalently to the atan2
    // angle, but is much faster to compute. See the following post for details:
    // https://stackoverflow.com/a/16561333/4316405.
    auto const routeSize = static_cast<double>(size());
    auto const dy = cumulatedY / routeSize - data->depot().y;
    auto const dx = cumulatedX / routeSize - data->depot().x;
    angleCenter = std::copysign(1. - dx / (std::fabs(dx) + std::fabs(dy)), dy);
}

void Route::setupRouteTimeWindows()
{
    auto const &dist = data->distanceMatrix();
    auto *node = nodes.back();

    do  // forward time window segments
    {
        auto *prev = p(node);
        prev->twAfter = TWS::merge(dist, prev->tw, node->twAfter);
        node = prev;
    } while (!node->isDepot());
}

bool Route::overlapsWith(Route const &other, int const tolerance) const
{
    return CircleSector::overlap(sector, other.sector, tolerance);
}

void Route::update()
{
    auto const oldNodes = nodes;
    setupNodes();

    auto const &dist = data->distanceMatrix();

    int load = 0;
    int distance = 0;
    int reverseDistance = 0;
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

        load += data->client(node->client).demand;
        distance += dist(p(node)->client, node->client);

        reverseDistance += dist(node->client, p(node)->client);
        reverseDistance -= dist(p(node)->client, node->client);

        node->position = pos + 1;
        node->cumulatedLoad = load;
        node->cumulatedDistance = distance;
        node->cumulatedReversalDistance = reverseDistance;
        node->twBefore = TWS::merge(dist, p(node)->twBefore, node->tw);
    }

    setupSector();
    setupRouteTimeWindows();

    load_ = nodes.back()->cumulatedLoad;
    isLoadFeasible_ = static_cast<size_t>(load_) <= data->vehicleCapacity();

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
