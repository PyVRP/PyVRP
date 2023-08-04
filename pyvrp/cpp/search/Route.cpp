#include "Route.h"

#include <cmath>
#include <numbers>
#include <ostream>
#include <utility>

using pyvrp::search::Route;
using TWS = pyvrp::TimeWindowSegment;

Route::Node::Node(size_t loc) : loc_(loc), idx_(0), route_(nullptr) {}

Route::Route(ProblemData const &data, size_t const idx, size_t const vehType)
    : data(data),
      vehicleType_(vehType),
      startDepot(data.vehicleType(vehType).depot),
      endDepot(data.vehicleType(vehType).depot),
      idx(idx)
{
    startDepot.route_ = this;
    startDepot.tw = TWS(startDepot.client(), data.client(startDepot.client()));

    endDepot.route_ = this;
    endDepot.tw = TWS(endDepot.client(), data.client(endDepot.client()));
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

    cumDist.clear();
    cumDist.push_back(0);
    cumDist.push_back(0);

    cumLoad.clear();
    cumLoad.push_back(0);
    cumLoad.push_back(0);

    startDepot.idx_ = 0;
    startDepot.twBefore = startDepot.tw;

    endDepot.idx_ = 1;
    endDepot.twAfter = endDepot.tw;
}

void Route::insert(size_t idx, Node *node)
{
    assert(0 < idx && idx < nodes.size());
    assert(!node->route());  // must previously have been unassigned

    node->idx_ = idx;
    node->route_ = this;

    cumDist.emplace_back();  // does not matter where we place these, as they
    cumLoad.emplace_back();  // will be updated by Route::update().

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

#ifdef PYVRP_NO_TIME_WINDOWS
    return;
#else
    // Backward time window segments (depot -> client)
    for (auto node = nodes.begin() + 1; node != nodes.end(); ++node)
        (*node)->twBefore = TWS::merge(
            data.durationMatrix(), p(*node)->twBefore, (*node)->tw);

    // Forward time window segments (client -> depot)
    // TODO std::ranges::view::reverse once clang supports it
    for (auto node = nodes.rbegin() + 1; node != nodes.rend(); ++node)
        (*node)->twAfter
            = TWS::merge(data.durationMatrix(), (*node)->tw, n(*node)->twAfter);
#endif
}

std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route)
{
    out << "Route #" << route.idx + 1 << ":";  // route number
    for (auto *node : route)
        out << ' ' << node->client();  // client index
    out << '\n';

    return out;
}
