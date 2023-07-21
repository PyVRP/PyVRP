#include "Route.h"

#include <cmath>
#include <numbers>
#include <ostream>

using pyvrp::search::Route;
using TWS = pyvrp::TimeWindowSegment;

Route::Node::Node(size_t client) : client(client) {}

void Route::Node::insertAfter(Route::Node *other)
{
    if (route)  // If we're in a route, we first stitch up the current route.
    {           // If we're not in a route, this step should be skipped.
        prev->next = next;
        next->prev = prev;
    }

    prev = other;
    next = other->next;

    other->next->prev = this;
    other->next = this;

    route = other->route;
}

void Route::Node::swapWith(Route::Node *other)
{
    auto *VPred = other->prev;
    auto *VSucc = other->next;
    auto *UPred = prev;
    auto *USucc = next;

    auto *routeU = route;
    auto *routeV = other->route;

    UPred->next = other;
    USucc->prev = other;
    VPred->next = this;
    VSucc->prev = this;

    prev = VPred;
    next = VSucc;
    other->prev = UPred;
    other->next = USucc;

    route = routeV;
    other->route = routeU;
}

void Route::Node::remove()
{
    prev->next = next;
    next->prev = prev;

    prev = nullptr;
    next = nullptr;
    route = nullptr;
}

Route::Route(ProblemData const &data, size_t const idx, size_t const vehType)
    : data(data),
      vehicleType_(vehType),
      idx(idx),
      startDepot(data.vehicleType(vehType).depot),
      endDepot(data.vehicleType(vehType).depot)
{
    startDepot.route = this;
    endDepot.route = this;
}

void Route::setupSector()
{
    if (empty())
        return;

    auto const &depotData = data.client(startDepot.client);
    auto const &clientData = data.client(n(&startDepot)->client);

    auto const diffX = static_cast<double>(clientData.x - depotData.x);
    auto const diffY = static_cast<double>(clientData.y - depotData.y);
    auto const angle = CircleSector::positive_mod(
        static_cast<int>(32768. * std::atan2(diffY, diffX) / std::numbers::pi));

    sector.initialize(angle);

    for (auto *node : *this)
    {
        auto const &clientData = data.client(node->client);

        auto const diffX = static_cast<double>(clientData.x - depotData.x);
        auto const diffY = static_cast<double>(clientData.y - depotData.y);
        auto const angle = CircleSector::positive_mod(static_cast<int>(
            32768. * std::atan2(diffY, diffX) / std::numbers::pi));

        sector.extend(angle);
    }
}

void Route::setupRouteTimeWindows()
{
    auto *node = &endDepot;

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
    nodes.clear();
    auto *node = n(&startDepot);

    Load load = 0;
    Distance distance = 0;
    Distance deltaReversalDistance = 0;

    while (!node->isDepot())
    {
        size_t const position = nodes.size();
        nodes.push_back(node);

        load += data.client(node->client).demand;
        distance += data.dist(p(node)->client, node->client);

        deltaReversalDistance += data.dist(node->client, p(node)->client);
        deltaReversalDistance -= data.dist(p(node)->client, node->client);

        node->position = position + 1;
        node->cumulatedLoad = load;
        node->cumulatedDistance = distance;
        node->deltaReversalDistance = deltaReversalDistance;
        node->twBefore
            = TWS::merge(data.durationMatrix(), p(node)->twBefore, node->tw);

        node = n(node);
    }

    load += data.client(endDepot.client).demand;
    distance += data.dist(p(&endDepot)->client, endDepot.client);

    deltaReversalDistance += data.dist(endDepot.client, p(&endDepot)->client);
    deltaReversalDistance -= data.dist(p(&endDepot)->client, endDepot.client);

    endDepot.position = size() + 1;
    endDepot.cumulatedLoad = load;
    endDepot.cumulatedDistance = distance;
    endDepot.deltaReversalDistance = deltaReversalDistance;
    endDepot.twBefore = TWS::merge(
        data.durationMatrix(), p(&endDepot)->twBefore, endDepot.tw);

    setupSector();
    setupRouteTimeWindows();

    load_ = endDepot.cumulatedLoad;
    timeWarp_ = endDepot.twBefore.totalTimeWarp();
}

std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route)
{
    out << "Route #" << route.idx + 1 << ":";  // route number
    for (auto *node : route)
        out << ' ' << node->client;  // client index
    out << '\n';

    return out;
}
