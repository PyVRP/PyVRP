#include "Route.h"

#include <cmath>
#include <numbers>
#include <ostream>
#include <utility>

using pyvrp::search::Route;
using TWS = pyvrp::TimeWindowSegment;

Route::Node::Node(size_t loc, ProblemData::Client const &client)
    : loc_(loc), idx_(0), route_(nullptr), tws_(loc, client)
{
}

Route::Route(ProblemData const &data, size_t idx, size_t vehicleType)
    : data(data),
      vehicleType_(vehicleType),
      idx_(idx),
      startDepot(data.vehicleType(vehicleType).depot,
                 data.client(data.vehicleType(vehicleType).depot)),
      endDepot(data.vehicleType(vehicleType).depot,
               data.client(data.vehicleType(vehicleType).depot))
{
    startDepot.route_ = this;
    endDepot.route_ = this;
    clear();
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

    startDepot.idx_ = 0;
    endDepot.idx_ = 1;

    cumDist.clear();
    cumDist.push_back(0);
    cumDist.push_back(0);

    cumLoad.clear();
    cumLoad.push_back(0);
    cumLoad.push_back(0);

    twsBefore.clear();
    twsAfter.clear();

    auto const startClient = startDepot.client();
    TWS const startTWS = TWS(startClient, data.client(startClient));

    auto const endClient = endDepot.client();
    TWS const endTWS = TWS(endClient, data.client(endClient));

    twsBefore.push_back(startTWS);
    twsBefore.push_back(TWS::merge(data.durationMatrix(), startTWS, endTWS));

    twsAfter.push_back(TWS::merge(data.durationMatrix(), startTWS, endTWS));
    twsAfter.push_back(endTWS);
}

void Route::insert(size_t idx, Node *node)
{
    assert(0 < idx && idx < nodes.size());
    assert(!node->route());  // must previously have been unassigned

    node->idx_ = idx;
    node->route_ = this;

    cumDist.emplace_back();  // does not matter where we place these, as they
    cumLoad.emplace_back();  // will be updated by Route::update().
    twsBefore.emplace_back();
    twsAfter.emplace_back();

    nodes.insert(nodes.begin() + idx, node);
    for (size_t after = idx; after != nodes.size(); ++after)
        nodes[after]->idx_ = after;
}

void Route::push_back(Node *node) { insert(size() + 1, node); }

void Route::remove(size_t idx)
{
    assert(0 < idx && idx < nodes.size() - 1);
    assert(nodes[idx]->route() == this);  // must currently be in this route

    auto *node = nodes[idx];

    node->idx_ = 0;
    node->route_ = nullptr;

    cumDist.pop_back();  // does not matter where we remove these, as they will
    cumLoad.pop_back();  // will be updated by Route::update().
    twsBefore.pop_back();
    twsAfter.pop_back();

    nodes.erase(nodes.begin() + idx);
    for (auto after = idx; after != nodes.size(); ++after)
        nodes[after]->idx_ = after;
}

void Route::swap(Node *first, Node *second)
{
    // TODO just swap clients?
    // TODO specialise std::swap for Node
    std::swap(first->route_->nodes[first->idx_],
              second->route_->nodes[second->idx_]);

    std::swap(first->route_, second->route_);
    std::swap(first->idx_, second->idx_);
}

void Route::update()
{
    centroid = {0, 0};

    for (size_t idx = 1; idx != nodes.size(); ++idx)
    {
        auto *node = nodes[idx];
        auto const &clientData = data.client(node->client());

        if (!node->isDepot())
        {
            centroid.first += static_cast<double>(clientData.x) / size();
            centroid.second += static_cast<double>(clientData.y) / size();
        }

        auto const dist = data.dist(p(node)->client(), node->client());
        cumDist[idx] = cumDist[idx - 1] + dist;
        cumLoad[idx] = cumLoad[idx - 1] + clientData.demand;
    }

#ifndef PYVRP_NO_TIME_WINDOWS
    // Backward time window segments (depot -> client).
    for (size_t idx = 1; idx != nodes.size(); ++idx)
        twsBefore[idx] = TWS::merge(
            data.durationMatrix(), twsBefore[idx - 1], nodes[idx]->tws());

    // Forward time window segments (client -> depot).
    for (auto idx = static_cast<int>(nodes.size() - 2); idx >= 0; --idx)
        twsAfter[idx] = TWS::merge(
            data.durationMatrix(), nodes[idx]->tws(), twsAfter[idx + 1]);
#endif
}

std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route)
{
    out << "Route #" << route.idx() + 1 << ":";  // route number
    for (auto *node : route)
        out << ' ' << node->client();  // client index
    out << '\n';

    return out;
}
