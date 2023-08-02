#include "Route.h"

#include <cmath>
#include <numbers>
#include <ostream>
#include <utility>

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

    startDepot.twBefore = startDepot.tw;
    endDepot.twAfter = endDepot.tw;
}

void Route::insert(size_t position, Node *node)
{
    assert(0 < position && position <= nodes.size() + 1);
    assert(!node->route);  // must previously have been unassigned

    auto *prev = position == 1 ? &startDepot : nodes[position - 2];

    node->position = position;
    node->route = prev->route;
    nodes.insert(nodes.begin() + position - 1, node);

    for (auto idx = position - 1; idx != nodes.size(); ++idx)
        nodes[idx]->position = idx + 1;
}

void Route::push_back(Node *node) { insert(size() + 1, node); }

void Route::remove(size_t position)
{
    assert(position > 0);
    auto *node = nodes[position - 1];

    node->route = nullptr;
    nodes.erase(nodes.begin() + position - 1);

    for (auto idx = position - 1; idx != nodes.size(); ++idx)
        nodes[idx]->position = idx + 1;
}

void Route::swap(Node *first, Node *second)
{
    // TODO just swap clients?
    // TODO specialise std::swap for Node
    std::swap(first->route->nodes[first->position - 1],
              second->route->nodes[second->position - 1]);

    std::swap(first->route, second->route);
    std::swap(first->position, second->position);
}

void Route::update()
{
    cumLoad.clear();
    cumDist.clear();

    centroid = {0, 0};
    load_ = 0;
    distance_ = 0;

    for (auto *node : nodes)
    {
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
    return;
#else
    // Backward time window segments (depot -> client)
    for (auto *node : nodes)
        node->twBefore
            = TWS::merge(data.durationMatrix(), p(node)->twBefore, node->tw);

    endDepot.twBefore = TWS::merge(
        data.durationMatrix(), p(&endDepot)->twBefore, endDepot.tw);

    // Forward time window segments (client -> depot)
    // TODO std::ranges::view::reverse once clang supports it
    for (auto node = nodes.rbegin(); node != nodes.rend(); ++node)
        (*node)->twAfter
            = TWS::merge(data.durationMatrix(), (*node)->tw, n(*node)->twAfter);

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
