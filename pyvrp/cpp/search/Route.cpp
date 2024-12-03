#include "Route.h"

#include <cmath>
#include <numbers>
#include <ostream>
#include <utility>

using pyvrp::search::Route;

Route::Node::Node(size_t loc, NodeType type)
    : loc_(loc), idx_(0), route_(nullptr), type_(type)
{
}

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
      idx_(idx),
      loadAt(data.numLoadDimensions()),
      tripLoadAfter(data.numLoadDimensions()),
      tripLoadBefore(data.numLoadDimensions()),
      tripLoad_(data.numLoadDimensions()),
      excessLoad_(data.numLoadDimensions())
{
    clear();
}

Route::~Route() { clear(); }

std::vector<Route::Node *>::const_iterator Route::begin() const
{
    return nodes.begin();
}
std::vector<Route::Node *>::const_iterator Route::end() const
{
    return nodes.end();
}

std::vector<Route::Node *>::iterator Route::begin() { return nodes.begin(); }
std::vector<Route::Node *>::iterator Route::end() { return nodes.end(); }

std::pair<double, double> const &Route::centroid() const
{
    assert(!dirty);
    return centroid_;
}

size_t Route::vehicleType() const
{
    auto const &vehicleTypes = data.vehicleTypes();
    return std::distance(&vehicleTypes[0], &vehicleType_);
}

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

    // Clear nodes and reinsert depot nodes of first trip.
    nodes.clear();
    depotNodes.clear();
    depotNodes.emplace_back(
        Node(vehicleType_.startDepot, Node::NodeType::DepotLoad),
        Node(vehicleType_.endDepot, Node::NodeType::DepotUnload));

    nodes.push_back(&depotNodes[0].first);
    nodes.push_back(&depotNodes[0].second);

    depotNodes[0].first.assign(this, 0);
    depotNodes[0].second.assign(this, 1);

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
    // Inserts before the last depot node.
    insert(nodes.size() - 1, node);

#ifndef NDEBUG
    dirty = true;
#endif
}

void Route::emplace_back_depot(size_t startDepotIdx, size_t endDepotIdx)
{
    assert(startDepotIdx == vehicleType_.startDepot
           && endDepotIdx == vehicleType_.endDepot);

    auto &depotPair = depotNodes.emplace_back(
        Node(startDepotIdx, Node::NodeType::DepotLoad),
        Node(endDepotIdx, Node::NodeType::DepotUnload));
    depotPair.first.assign(this, nodes.size());
    depotPair.second.assign(this, nodes.size() + 1);
    nodes.push_back(&depotPair.first);
    nodes.push_back(&depotPair.second);

#ifndef NDEBUG
    dirty = true;
#endif
}

void Route::remove(size_t idx)
{
    // TODO: remove trip depot nodes if the trip becomes empty (except if it is
    // the only trip).
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
    // TODO not allowed to swap depot nodes?
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
    assert(nodes.size() >= 2);  // Depot nodes of first trip must be included.

    size_t numClients = 0;
    visits.clear();
    for (auto const *node : nodes)
    {
        visits.emplace_back(node->client());
        if (node->type() == Node::NodeType::Client)
            ++numClients;
    }

    centroid_ = {0, 0};
    for (size_t idx = 1; idx != nodes.size() - 1; ++idx)
    {
        if (nodes[idx]->isDepot())
            continue;

        ProblemData::Client const &clientData = data.location(visits[idx]);
        centroid_.first += static_cast<double>(clientData.x) / numClients;
        centroid_.second += static_cast<double>(clientData.y) / numClients;
    }

    // Trip idx.
    tripIdx.resize(nodes.size());
    tripIdx[0] = 0;
    for (size_t idx = 1; idx != nodes.size(); ++idx)
        tripIdx[idx] = nodes[idx]->type() == Node::NodeType::DepotLoad
                           ? tripIdx[idx - 1] + 1
                           : tripIdx[idx - 1];

    size_t numTrips = tripIdx.back() + 1;

    // Distance.
    auto const &distMat = data.distanceMatrix(profile());

    cumDist.resize(nodes.size());
    cumDist[0] = 0;
    for (size_t idx = 1; idx != nodes.size(); ++idx)
        cumDist[idx] = cumDist[idx - 1] + distMat(visits[idx - 1], visits[idx]);

#ifndef PYVRP_NO_TIME_WINDOWS
    // Duration.
    durAt.resize(nodes.size());
    durAt[0] = {vehicleType_, vehicleType_.startLate};
    for (size_t idx = 1; idx != nodes.size(); ++idx)
        durAt[idx] = nodes[idx]->type() == Node::NodeType::Client
                         ? DurationSegment(data.location(visits[idx]))
                         : DurationSegment(vehicleType_, vehicleType_.twLate);

    auto const &durMat = data.durationMatrix(profile());

    durBefore.resize(nodes.size());
    durBefore[0] = durAt[0];
    for (size_t idx = 1; idx != nodes.size(); ++idx)
        durBefore[idx]
            = DurationSegment::merge(durMat(visits[idx - 1], visits[idx]),
                                     durBefore[idx - 1],
                                     durAt[idx]);

    durAfter.resize(nodes.size());
    durAfter[nodes.size() - 1] = durAt[nodes.size() - 1];
    for (size_t idx = nodes.size() - 1; idx != 0; --idx)
        durAfter[idx - 1]
            = DurationSegment::merge(durMat(visits[idx - 1], visits[idx]),
                                     durAt[idx - 1],
                                     durAfter[idx]);
#endif

    // Load.
    for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
    {
        loadAt[dim].resize(nodes.size());
        for (size_t idx = 0; idx != nodes.size(); ++idx)
            loadAt[dim][idx]
                = nodes[idx]->type() == Node::NodeType::Client
                      ? LoadSegment(data.location(visits[idx]), dim)
                      : LoadSegment();

        tripLoadBefore[dim].resize(nodes.size());
        tripLoad_[dim].resize(numTrips);
        for (size_t idx = 0; idx != nodes.size(); ++idx)
        {
            if (nodes[idx]->type() == Node::NodeType::DepotLoad)
                tripLoadBefore[dim][idx] = loadAt[dim][idx];
            else
                tripLoadBefore[dim][idx] = LoadSegment::merge(
                    tripLoadBefore[dim][idx - 1], loadAt[dim][idx]);

            // Update the load per trip.
            if (nodes[idx]->type() == Node::NodeType::DepotUnload)
                tripLoad_[dim][tripIdx[idx]] = tripLoadBefore[dim][idx].load();
        }

        excessLoad_[dim] = 0;
        for (size_t trip = 0; trip != numTrips; ++trip)
        {
            Load const tripLoad = tripLoad_[dim][trip];
            excessLoad_[dim] += std::max<Load>(tripLoad - capacity()[dim], 0);
        }

        tripLoadAfter[dim].resize(nodes.size());
        tripLoadAfter[dim][nodes.size() - 1] = loadAt[dim][nodes.size() - 1];
        for (size_t idx = nodes.size() - 1; idx != 0; --idx)
        {
            if (nodes[idx - 1]->type() == Node::NodeType::DepotUnload)
                tripLoadAfter[dim][idx - 1] = loadAt[dim][idx - 1];
            else
                tripLoadAfter[dim][idx - 1] = LoadSegment::merge(
                    loadAt[dim][idx - 1], tripLoadAfter[dim][idx]);
        }
    }

#ifndef NDEBUG
    dirty = false;
#endif
}

std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route)
{
    out << "Route #" << route.idx() + 1 << ":";  // route number
    for (auto *node : route)
    {
        if (node->type() == Route::Node::NodeType::Client)
            out << ' ' << node->client();  // client index
        else if (node->type() == Route::Node::NodeType::DepotLoad
                 && node != *route.begin())
            out << " |";  // trip delimiter
    }
    out << '\n';

    return out;
}
