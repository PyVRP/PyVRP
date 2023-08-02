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
    nodes.push_back(&startDepot);
    nodes.push_back(&endDepot);

    startDepot.twBefore = startDepot.tw;
    endDepot.twAfter = endDepot.tw;
}

void Route::insert(size_t position, Node *node)
{
    assert(0 < position && position < nodes.size());
    assert(!node->route);  // must previously have been unassigned

    auto *prev = nodes[position - 1];

    node->position = position;
    node->route = prev->route;
    nodes.insert(nodes.begin() + position, node);

    for (auto idx = position; idx != nodes.size(); ++idx)
        nodes[idx]->position = idx;
}

void Route::push_back(Node *node) { insert(size() + 1, node); }

void Route::remove(size_t position)
{
    assert(0 < position && position < nodes.size() - 1);
    auto *node = nodes[position];

    node->route = nullptr;
    nodes.erase(nodes.begin() + position);

    for (auto idx = position; idx != nodes.size(); ++idx)
        nodes[idx]->position = idx;
}

void Route::swap(Node *first, Node *second)
{
    // TODO just swap clients?
    // TODO specialise std::swap for Node
    std::swap(first->route->nodes[first->position],
              second->route->nodes[second->position]);

    std::swap(first->route, second->route);
    std::swap(first->position, second->position);
}

void Route::update()
{
    cumLoad.clear();
    cumDist.clear();

    centroid = {0, 0};

    Load load = 0;
    Distance distance = 0;

    for (auto *node : nodes)
    {
        auto const &clientData = data.client(node->client);

        if (!node->isDepot())
        {
            centroid.first += static_cast<double>(clientData.x);
            centroid.second += static_cast<double>(clientData.y);
        }

        load += clientData.demand;
        cumLoad.push_back(load);

        distance += data.dist(p(node)->client, node->client);
        cumDist.push_back(distance);
    }

    centroid.first /= size();
    centroid.second /= size();

#ifdef PYVRP_NO_TIME_WINDOWS
    return;
#else
    // Backward time window segments (depot -> client)
    for (auto node = nodes.begin() + 1; node != nodes.end(); ++node)
        (*node)->twBefore = TWS::merge(
            data.durationMatrix(), p(*node)->twBefore, (*node)->tw);

    // Forward time window segments (client -> depot)
    // TODO std::ranges::view::reverse once clang supports it
    for (auto node = nodes.rbegin() - 1; node != nodes.rend(); ++node)
        (*node)->twAfter
            = TWS::merge(data.durationMatrix(), (*node)->tw, n(*node)->twAfter);
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
