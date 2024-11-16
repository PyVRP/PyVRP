#include "Route.h"

#include <cmath>
#include <numbers>
#include <ostream>
#include <utility>

using pyvrp::search::Route;

Route::Node::Node(size_t loc) : loc_(loc), idx_(0), route_(nullptr) {}

void Route::Node::assign(Route *route, size_t idx)
{
    assert(!route_);  // must not currently be assigned
    idx_ = idx;
    route_ = route;
}

void Route::Node::unassign()
{
    idx_ = 0;
    route_ = nullptr;
}

Route::Route(ProblemData const &data, size_t idx, size_t vehicleType)
    : data(data),
      vehicleType_(data.vehicleType(vehicleType)),
      vehTypeIdx_(vehicleType),
      idx_(idx),
      startDepot_(vehicleType_.startDepot),
      endDepot_(vehicleType_.endDepot),
      loadAt(data.numLoadDimensions()),
      loadAfter(data.numLoadDimensions()),
      loadBefore(data.numLoadDimensions()),
      load_(data.numLoadDimensions()),
      excessLoad_(data.numLoadDimensions())
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
    for (auto *node : nodes)
        node->unassign();

    nodes.clear();  // clear nodes and reinsert the depots.
    nodes.push_back(&startDepot_);
    nodes.push_back(&endDepot_);

    startDepot_.assign(this, 0);
    endDepot_.assign(this, 1);

    update();
}

void Route::insert(size_t idx, Node *node)
{
    assert(0 < idx && idx < nodes.size());

    node->assign(this, idx);
    nodes.insert(nodes.begin() + idx, node);

    for (size_t after = idx; after != nodes.size(); ++after)
        nodes[after]->idx_ = after;

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

    nodes[idx]->unassign();
    nodes.erase(nodes.begin() + idx);

    for (auto after = idx; after != nodes.size(); ++after)
        nodes[after]->idx_ = after;

#ifndef NDEBUG
    dirty = true;
#endif
}

void Route::swap(Node *first, Node *second)
{
    // TODO specialise std::swap for Node
    if (first->route_)
        first->route_->nodes[first->idx_] = second;

    if (second->route_)
        second->route_->nodes[second->idx_] = first;

    std::swap(first->route_, second->route_);
    std::swap(first->idx_, second->idx_);

#ifndef NDEBUG
    if (first->route_)
        first->route_->dirty = true;

    if (second->route_)
        second->route_->dirty = true;
#endif
}

void Route::update()
{
    visits.clear();
    for (auto const *node : nodes)
        visits.emplace_back(node->client());

    centroid_ = {0, 0};
    for (size_t idx = 1; idx != nodes.size() - 1; ++idx)
    {
        ProblemData::Client const &clientData = data.location(visits[idx]);
        centroid_.first += static_cast<double>(clientData.x) / size();
        centroid_.second += static_cast<double>(clientData.y) / size();
    }

    // Distance.
    auto const &distMat = data.distanceMatrix(profile());

    distBefore.resize(nodes.size());
    distBefore[0] = {0};
    for (size_t idx = 1; idx != nodes.size(); ++idx)
        distBefore[idx] = DistanceSegment::merge(
            distMat(visits[idx - 1], visits[idx]), distBefore[idx - 1], {0});

    distAfter.resize(nodes.size());
    distAfter[nodes.size() - 1] = {0};
    for (size_t idx = nodes.size() - 1; idx != 0; --idx)
        distAfter[idx - 1] = DistanceSegment::merge(
            distMat(visits[idx - 1], visits[idx]), {0}, distAfter[idx]);

    durAt.resize(nodes.size());
    durBefore.resize(nodes.size());
    durAfter.resize(nodes.size());
    if (data.characteristics().hasDuration)  // Duration.
    {
        durAt[0] = {vehicleType_};
        durAt[nodes.size() - 1] = {vehicleType_};

        for (size_t idx = 1; idx != nodes.size() - 1; ++idx)
            durAt[idx] = {data.location(visits[idx])};

        auto const &durMat = data.durationMatrix(profile());

        durBefore[0] = durAt[0];
        for (size_t idx = 1; idx != nodes.size(); ++idx)
            durBefore[idx]
                = DurationSegment::merge(durMat(visits[idx - 1], visits[idx]),
                                         durBefore[idx - 1],
                                         durAt[idx]);

        durAfter[nodes.size() - 1] = durAt[nodes.size() - 1];
        for (size_t idx = nodes.size() - 1; idx != 0; --idx)
            durAfter[idx - 1]
                = DurationSegment::merge(durMat(visits[idx - 1], visits[idx]),
                                         durAt[idx - 1],
                                         durAfter[idx]);
    }

    // Load.
    for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
    {
        loadAt[dim].resize(nodes.size());
        loadAt[dim][0] = {};
        loadAt[dim][nodes.size() - 1] = {};

        for (size_t idx = 1; idx != nodes.size() - 1; ++idx)
            loadAt[dim][idx] = {data.location(visits[idx]), dim};

        loadBefore[dim].resize(nodes.size());
        loadBefore[dim][0] = loadAt[dim][0];
        for (size_t idx = 1; idx != nodes.size(); ++idx)
            loadBefore[dim][idx] = LoadSegment::merge(loadBefore[dim][idx - 1],
                                                      loadAt[dim][idx]);

        load_[dim] = loadBefore[dim].back().load();
        excessLoad_[dim] = std::max<Load>(load_[dim] - capacity()[dim], 0);

        loadAfter[dim].resize(nodes.size());
        loadAfter[dim][nodes.size() - 1] = loadAt[dim][nodes.size() - 1];
        for (size_t idx = nodes.size() - 1; idx != 0; --idx)
            loadAfter[dim][idx - 1]
                = LoadSegment::merge(loadAt[dim][idx - 1], loadAfter[dim][idx]);
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
