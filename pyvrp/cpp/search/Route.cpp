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

Route::Route(ProblemData const &data, size_t const idx, size_t const vehType)
    : data(data),
      vehicleType_(vehType),
      idx(idx),
      startDepot(data.vehicleType(vehType).depot),
      endDepot(data.vehicleType(vehType).depot)
{
    startDepot.route = this;
    startDepot.tw = TWS(startDepot.client, data.client(startDepot.client));

    endDepot.route = this;
    endDepot.tw = TWS(endDepot.client, data.client(endDepot.client));
}

size_t Route::vehicleType() const { return vehicleType_; }

bool Route::overlapsWith(Route const &other, double tolerance) const
{
    auto const [dataX, dataY] = data.centroid();
    auto const [thisX, thisY] = centroid;
    auto const [otherX, otherY] = other.centroid;

    // Each angle is in [-pi, pi], so the absolute difference is in [0, tau].
    auto const thisAngle = std::atan2(thisY - dataY, thisX - dataX);
    auto const otherAngle = std::atan2(otherY - dataY, otherX - dataX);
    auto const absDiff = std::abs(thisAngle - otherAngle);

    // First case is obvious. Second case exists because tau and 0 are also
    // close together but separated by one period.
    auto constexpr tau = 2 * std::numbers::pi;
    return absDiff <= tolerance * tau || absDiff >= (1 - tolerance) * tau;
}

void Route::clear()
{
    nodes.clear();

    startDepot.prev = &endDepot;
    startDepot.next = &endDepot;
    startDepot.twBefore = startDepot.tw;

    endDepot.prev = &startDepot;
    endDepot.next = &startDepot;
    endDepot.twAfter = endDepot.tw;
}

void Route::push_back(Node *node)
{
    node->insertAfter(empty() ? &startDepot : nodes.back());
    nodes.push_back(node);
}

void Route::remove(size_t position)
{
    assert(position > 0);
    auto *node = nodes[position - 1];

    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->prev = nullptr;
    node->next = nullptr;
    node->route = nullptr;

    nodes.erase(nodes.begin() + position - 1);
}

void Route::update()
{
    nodes.clear();
    cumLoad.clear();
    cumDist.clear();

    centroid = {0, 0};
    load_ = 0;
    distance_ = 0;

    for (auto *node = n(&startDepot); !node->isDepot(); node = n(node))
    {
        node->position = size() + 1;
        nodes.push_back(node);

        auto const &clientData = data.client(node->client);

        centroid.first += static_cast<double>(clientData.x);
        centroid.second += static_cast<double>(clientData.y);

        load_ += clientData.demand;
        cumLoad.push_back(load_);

        distance_ += data.dist(p(node)->client, node->client);
        cumDist.push_back(distance_);
    }

    endDepot.position = size() + 1;

    centroid.first /= size();
    centroid.second /= size();

    load_ += data.client(endDepot.client).demand;
    distance_ += data.dist(p(&endDepot)->client, endDepot.client);

#ifdef PYVRP_NO_TIME_WINDOWS
    timeWarp_ = 0;
    return;
#else
    // Backward time window segments
    for (auto *node = n(&startDepot); !node->isDepot(); node = n(node))
        node->twBefore
            = TWS::merge(data.durationMatrix(), p(node)->twBefore, node->tw);

    endDepot.twBefore = TWS::merge(
        data.durationMatrix(), p(&endDepot)->twBefore, endDepot.tw);

    timeWarp_ = endDepot.twBefore.totalTimeWarp();

    // Forward time window segments
    auto *node = &endDepot;
    do
    {
        auto *prev = p(node);
        prev->twAfter
            = TWS::merge(data.durationMatrix(), prev->tw, node->twAfter);
        node = prev;
    } while (!node->isDepot());
#endif
}

std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route)
{
    out << "Route #" << route.idx + 1 << ":";  // route number
    for (auto *node : route)
        out << ' ' << node->client;  // client index
    out << '\n';

    return out;
}
