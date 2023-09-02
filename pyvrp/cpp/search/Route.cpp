#include "Route.h"

#include <cmath>
#include <numbers>
#include <ostream>
#include <utility>

using pyvrp::search::Route;
using TWS = pyvrp::TimeWindowSegment;

Route::Node::Node(size_t loc) : loc_(loc), idx_(0), route_(nullptr) {}

Route::Route(ProblemData const &data, size_t idx, size_t vehicleType)
    : data(data),
      vehicleType_(data.vehicleType(vehicleType)),
      vehTypeIdx_(vehicleType),
      idx_(idx),
      startDepot(vehicleType_.depot),
      endDepot(vehicleType_.depot)
{
    clear();
}

Route::~Route() { clear(); }

Route::NodeStats::NodeStats(TimeWindowSegment const &tws)
    : cumDist(0), cumLoad(0), tws(tws), twsAfter(tws), twsBefore(tws)
{
}

size_t Route::vehicleType() const { return vehTypeIdx_; }

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
    for (auto *node : nodes)  // unassign all nodes from route.
    {
        node->idx_ = 0;
        node->route_ = nullptr;
    }

    nodes.clear();  // clear nodes and reinsert the depots.
    nodes.push_back(&startDepot);
    nodes.push_back(&endDepot);

    startDepot.idx_ = 0;
    startDepot.route_ = this;

    endDepot.idx_ = 1;
    endDepot.route_ = this;

    auto const &depot = data.client(vehicleType_.depot);

    // Time window is limited by both the depot open and closing times, and
    // the vehicle's start and end of shift, whichever is tighter. If the
    // vehicle does not have a shift time window, we default to the depot's
    // open and close times.
    auto const shiftStart = vehicleType_.twEarly.value_or(depot.twEarly);
    auto const shiftEnd = vehicleType_.twLate.value_or(depot.twLate);

    TWS depotTws(vehicleType_.depot,
                 vehicleType_.depot,
                 0,
                 0,
                 std::max(depot.twEarly, shiftStart),
                 std::min(depot.twLate, shiftEnd),
                 0);

    stats.clear();  // clear stats and reinsert depot statistics.
    stats.emplace_back(depotTws);
    stats.emplace_back(depotTws);
}

void Route::insert(size_t idx, Node *node)
{
    assert(0 < idx && idx < nodes.size());
    assert(!node->route());  // must previously have been unassigned

    node->idx_ = idx;
    node->route_ = this;
    nodes.insert(nodes.begin() + idx, node);

    // We do not need to update the statistics; Route::update() will handle
    // that later.
    stats.insert(stats.begin() + idx,
                 TWS(node->client(), data.client(node->client())));

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

    nodes.erase(nodes.begin() + idx);
    stats.erase(stats.begin() + idx);

    for (auto after = idx; after != nodes.size(); ++after)
        nodes[after]->idx_ = after;
}

void Route::swap(Node *first, Node *second)
{
    // TODO specialise std::swap for Node
    std::swap(first->route_->nodes[first->idx_],
              second->route_->nodes[second->idx_]);
    std::swap(first->route_->stats[first->idx_],
              second->route_->stats[second->idx_]);

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

        auto const dist = data.dist(nodes[idx - 1]->client(), node->client());
        stats[idx].cumDist = stats[idx - 1].cumDist + dist;
        stats[idx].cumLoad = stats[idx - 1].cumLoad + clientData.demand;
    }

#ifndef PYVRP_NO_TIME_WINDOWS
    // Backward time window segments (depot -> client).
    for (size_t idx = 1; idx != nodes.size(); ++idx)
        stats[idx].twsBefore = TWS::merge(
            data.durationMatrix(), stats[idx - 1].twsBefore, stats[idx].tws);

    // Forward time window segments (client -> depot).
    for (auto idx = nodes.size() - 1; idx != 0; --idx)
        stats[idx - 1].twsAfter = TWS::merge(
            data.durationMatrix(), stats[idx - 1].tws, stats[idx].twsAfter);
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
