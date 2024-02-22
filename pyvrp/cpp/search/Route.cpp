#include "Route.h"

#include <cmath>
#include <numbers>
#include <ostream>
#include <utility>

using pyvrp::search::Route;

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

std::vector<Route::Node *>::const_iterator Route::begin() const
{
    return nodes.begin() + 1;
}
std::vector<Route::Node *>::const_iterator Route::end() const
{
    return nodes.end() - 1;
}

std::vector<Route::Node *>::iterator Route::begin()
{
    return nodes.begin() + 1;
}
std::vector<Route::Node *>::iterator Route::end() { return nodes.end() - 1; }

pyvrp::Distance Route::distance() const
{
    assert(!dirty);
    return cumDist.back();
}

pyvrp::Duration Route::duration() const
{
    assert(!dirty);
    return dsBefore.back().duration();
}

std::pair<double, double> const &Route::centroid() const
{
    assert(!dirty);
    return centroid_;
}

size_t Route::vehicleType() const { return vehTypeIdx_; }

bool Route::overlapsWith(Route const &other, double tolerance) const
{
    assert(!dirty && !other.dirty);

    auto const [dataX, dataY] = data.centroid();
    auto const [thisX, thisY] = centroid_;
    auto const [otherX, otherY] = other.centroid_;

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

    // Time window is limited by both the depot open and closing times, and
    // the vehicle's start and end of shift, whichever is tighter.
    ProblemData::Depot const &depot = data.location(vehicleType_.depot);
    DurationSegment depotDS(vehicleType_.depot,
                            vehicleType_.depot,
                            0,
                            0,
                            std::max(depot.twEarly, vehicleType_.twEarly),
                            std::min(depot.twLate, vehicleType_.twLate),
                            0);

    // Clear all existing statistics and reinsert depot statistics.
    cumDist = {0, 0};

    ls = {LoadSegment(0, 0, 0), LoadSegment(0, 0, 0)};
    lsAfter = ls;
    lsBefore = ls;

    ds = {depotDS, depotDS};
    dsAfter = ds;
    dsBefore = ds;

#ifndef NDEBUG
    dirty = false;
#endif
}

void Route::insert(size_t idx, Node *node)
{
    assert(0 < idx && idx < nodes.size());
    assert(!node->route());  // must previously have been unassigned
    assert(!node->isDepot());

    node->idx_ = idx;
    node->route_ = this;
    nodes.insert(nodes.begin() + idx, node);

    for (size_t after = idx; after != nodes.size(); ++after)
        nodes[after]->idx_ = after;

    // We do not need to update the statistics; Route::update() will handle
    // that later. We just need to ensure the right client data is inserted.
    cumDist.push_back(0);  // no need for correct index

    ProblemData::Client const &client = data.location(node->client());

    ls.emplace(ls.begin() + idx, client);
    lsAfter.emplace_back(client);   // no need for correct index
    lsBefore.emplace_back(client);  // no need for correct index

    ds.emplace(ds.begin() + idx, node->client(), client);
    dsAfter.emplace_back(node->client(), client);   // no need for correct index
    dsBefore.emplace_back(node->client(), client);  // no need for correct index

#ifndef NDEBUG
    dirty = true;
#endif
}

void Route::push_back(Node *node)
{
    insert(size() + 1, node);

#ifndef NDEBUG
    dirty = true;
#endif
}

void Route::remove(size_t idx)
{
    assert(0 < idx && idx < nodes.size() - 1);
    assert(nodes[idx]->route() == this);  // must currently be in this route

    auto *node = nodes[idx];

    node->idx_ = 0;
    node->route_ = nullptr;

    nodes.erase(nodes.begin() + idx);

    cumDist.pop_back();  // no need for correct index

    ls.erase(ls.begin() + idx);
    lsBefore.pop_back();  // no need for correct index
    lsAfter.pop_back();   // no need for correct index

    ds.erase(ds.begin() + idx);
    dsBefore.pop_back();  // no need for correct index
    dsAfter.pop_back();   // no need for correct index

    for (auto after = idx; after != nodes.size(); ++after)
        nodes[after]->idx_ = after;

#ifndef NDEBUG
    dirty = true;
#endif
}

void Route::swap(Node *first, Node *second)
{
    // TODO specialise std::swap for Node
    std::swap(first->route_->nodes[first->idx_],
              second->route_->nodes[second->idx_]);

    std::swap(first->route_->ls[first->idx_], second->route_->ls[second->idx_]);
    std::swap(first->route_->ds[first->idx_], second->route_->ds[second->idx_]);

    std::swap(first->route_, second->route_);
    std::swap(first->idx_, second->idx_);

#ifndef NDEBUG
    first->route_->dirty = true;
    second->route_->dirty = true;
#endif
}

void Route::update()
{
    centroid_ = {0, 0};

    for (size_t idx = 1; idx != nodes.size(); ++idx)
    {
        auto const *node = nodes[idx];
        size_t const client = node->client();

        if (!node->isDepot())
        {
            ProblemData::Client const &clientData = data.location(client);
            centroid_.first += static_cast<double>(clientData.x) / size();
            centroid_.second += static_cast<double>(clientData.y) / size();
        }

        auto const dist = data.dist(nodes[idx - 1]->client(), client);
        cumDist[idx] = cumDist[idx - 1] + dist;
    }

    // Backward segments (depot -> client).
    for (size_t idx = 1; idx != nodes.size(); ++idx)
    {
        lsBefore[idx] = LoadSegment::merge(lsBefore[idx - 1], ls[idx]);

#ifndef PYVRP_NO_TIME_WINDOWS
        dsBefore[idx] = DurationSegment::merge(
            data.durationMatrix(), dsBefore[idx - 1], ds[idx]);
#endif
    }

    // Forward segments (client -> depot).
    for (auto idx = nodes.size() - 1; idx != 0; --idx)
    {
        lsAfter[idx - 1] = LoadSegment::merge(ls[idx - 1], lsAfter[idx]);

#ifndef PYVRP_NO_TIME_WINDOWS
        dsAfter[idx - 1] = DurationSegment::merge(
            data.durationMatrix(), ds[idx - 1], dsAfter[idx]);
#endif
    }

#ifndef NDEBUG
    dirty = false;
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
