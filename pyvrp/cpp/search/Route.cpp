#include "Route.h"

#include <cassert>
#include <cmath>
#include <numbers>
#include <ostream>

using pyvrp::search::Route;
using TWS = pyvrp::TimeWindowSegment;

Route::Node::Node(size_t client) : client(client) {}

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

void Route::insert(size_t position, Node *node)
{
    assert(position > 0);
    auto *prev = position == 1 ? &startDepot : nodes[position - 2];

    if (node->route)
    {
        // TODO use Route::remove()
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    node->prev = prev;
    node->next = prev->next;
    node->route = prev->route;

    prev->next->prev = node;
    prev->next = node;

    node->position = position;
    nodes.insert(nodes.begin() + position - 1, node);
}

void Route::push_back(Node *node) { insert(size() + 1, node); }

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

void Route::swap(Node *first, Node *second)
{
    auto *fPred = first->prev;
    auto *fSucc = first->next;
    auto *sPred = second->prev;
    auto *sSucc = second->next;

    auto *fRoute = first->route;
    auto *sRoute = second->route;

    fPred->next = second;
    fSucc->prev = second;
    sPred->next = first;
    sSucc->prev = first;

    first->prev = sPred;
    first->next = sSucc;
    second->prev = fPred;
    second->next = fSucc;

    first->route = sRoute;
    second->route = fRoute;
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
    // Backward time window segments (depot -> client)
    for (auto *node = n(&startDepot); !node->isDepot(); node = n(node))
        node->twBefore
            = TWS::merge(data.durationMatrix(), p(node)->twBefore, node->tw);

    endDepot.twBefore = TWS::merge(
        data.durationMatrix(), p(&endDepot)->twBefore, endDepot.tw);

    timeWarp_ = endDepot.twBefore.totalTimeWarp();

    // Forward time window segments (client -> depot)
    for (auto *node = p(&endDepot); !node->isDepot(); node = p(node))
        node->twAfter
            = TWS::merge(data.durationMatrix(), node->tw, n(node)->twAfter);

    startDepot.twAfter = TWS::merge(
        data.durationMatrix(), startDepot.tw, n(&startDepot)->twAfter);
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
