#define _USE_MATH_DEFINES  // needed to get M_PI etc. on Windows builds
// TODO use std::numbers::pi instead of M_PI when C++20 is supported by CIBW

#include "Route.h"

#include <cassert>
#include <cmath>
#include <ostream>

using TWS = TimeWindowSegment;

Route::Route(ProblemData const &data) : data(data), centroid({0, 0}) {}

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

void Route::setupCentroid()
{
    centroid = {0, 0};

    for (auto it = nodes.begin(); it != nodes.end() - 1; ++it)
    {
        auto const *node = *it;
        assert(!node->isDepot());

        auto const &clientData = data.client(node->client);
        centroid.first += static_cast<double>(clientData.x) / size();
        centroid.second += static_cast<double>(clientData.y) / size();
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

bool Route::overlapsWith(Route const &other, double tolerance) const
{
    assert(0 <= tolerance && tolerance <= 1);

    auto const [dataX, dataY] = data.centroid();
    auto const [thisX, thisY] = centroid;
    auto const [otherX, otherY] = other.centroid;

    // Each angle is in [-pi, pi], since that's what atan2 returns. So if the
    // tolerance is 100%, we should allow values up to 2 * pi.
    auto const thisAngle = std::atan2(thisY - dataY, thisX - dataX);
    auto const otherAngle = std::atan2(otherY - dataY, otherX - dataX);
    return std::abs(thisAngle - otherAngle) <= tolerance * 2 * M_PI;
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

    setupCentroid();
    setupRouteTimeWindows();

    load_ = nodes.back()->cumulatedLoad;
    isLoadFeasible_ = static_cast<size_t>(load_) <= data.vehicleCapacity();

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
