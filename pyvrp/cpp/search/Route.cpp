#include "Route.h"

#include <cmath>
#include <numbers>
#include <ostream>
#include <utility>

using pyvrp::search::Route;

Route::Node::Node(size_t loc) : loc_(loc), idx_(0), trip_(0), route_(nullptr) {}

void Route::Node::assign(Route *route, size_t idx, size_t trip)
{
    idx_ = idx;
    trip_ = trip;
    route_ = route;
}

void Route::Node::unassign()
{
    idx_ = 0;
    trip_ = 0;
    route_ = nullptr;
}

Route::Iterator::Iterator(std::vector<Node *> const &nodes, size_t idx)
    : nodes_(&nodes), idx_(idx)
{
    ensureValidIndex();
}

void Route::Iterator::ensureValidIndex()
{
    // size() - 1 is the index of the end depot, and what's returned by
    // Route::end() - we must not exceed it.
    while (idx_ < nodes_->size() - 1 && operator*() -> isReloadDepot())
        idx_++;  // skip any intermediate reload depots

    assert(0 < idx_ && idx_ < nodes_->size());
}

bool Route::Iterator::operator==(Iterator const &other) const
{
    return nodes_ == other.nodes_ && idx_ == other.idx_;
}

Route::Node *Route::Iterator::operator*() const { return (*nodes_)[idx_]; }

Route::Iterator Route::Iterator::operator++(int)
{
    auto tmp = *this;
    ++*this;
    return tmp;
}

Route::Iterator &Route::Iterator::operator++()
{
    idx_++;
    ensureValidIndex();
    return *this;
}

Route::Route(ProblemData const &data, size_t idx, size_t vehicleType)
    : data(data),
      vehicleType_(data.vehicleType(vehicleType)),
      idx_(idx),
      loadAt(data.numLoadDimensions()),
      loadAfter(data.numLoadDimensions()),
      loadBefore(data.numLoadDimensions()),
      load_(data.numLoadDimensions()),
      excessLoad_(data.numLoadDimensions())
{
    clear();
}

Route::~Route() { clear(); }

Route::Iterator Route::begin() const { return Iterator(nodes, 1); }

Route::Iterator Route::end() const { return Iterator(nodes, nodes.size() - 1); }

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
    if (nodes.size() == 2)  // then the route is already empty and we have
        return;             // nothing to do.

    for (auto *node : nodes)
        node->unassign();

    nodes.clear();
    depots_.clear();

    depots_.emplace_back(vehicleType_.startDepot);
    depots_.emplace_back(vehicleType_.endDepot);

    for (size_t idx : {0, 1})
    {
        nodes.push_back(&depots_[idx]);
        depots_[idx].assign(this, idx, idx);
    }

    update();
    assert(empty());
}

void Route::reserve(size_t size) { nodes.reserve(size); }

void Route::insert(size_t idx, Node *node)
{
    assert(0 < idx && idx < nodes.size());
    auto const isDepot = node->client() < data.numDepots();

    if (isDepot)  // is depot, so we need to insert a copy into our own memory
    {
        if (depots_.size() == depots_.capacity())  // then we reallocate and
        {                                          // must update references
            depots_.reserve(depots_.size() + 1);
            for (auto &depot : depots_)
                nodes[depot.idx()] = &depot;
        }

        node = &depots_.emplace_back(node->client());
    }

    if (numTrips() > maxTrips())
        throw std::invalid_argument("Vehicle cannot perform this many trips.");

    nodes.insert(nodes.begin() + idx, node);
    node->assign(this, idx, nodes[idx - 1]->trip());

    for (size_t after = idx; after != nodes.size(); ++after)
    {
        nodes[after]->idx_ = after;
        if (isDepot)  // then we need to bump each following trip index
            nodes[after]->trip_++;
    }
}

void Route::push_back(Node *node) { insert(nodes.size() - 1, node); }

void Route::remove(size_t idx)
{
    assert(0 < idx && idx < nodes.size() - 1);  // is not start or end depot
    assert(nodes[idx]->route() == this);        // must be in this route
    auto const isDepot = nodes[idx]->isReloadDepot();

    if (isDepot)
    {
        // We own this node - it's in our depots vector. We erase it, and then
        // update reload depot references that were invalidated by the erasure.
        auto const depotIdx = std::distance(depots_.data(), nodes[idx]);
        auto it = depots_.erase(depots_.begin() + depotIdx);
        for (; it != depots_.end(); ++it)
            nodes[it->idx()] = &*it;
    }
    else
        // We do not own this node, so we only unassign it.
        nodes[idx]->unassign();

    nodes.erase(nodes.begin() + idx);  // remove dangling pointer
    for (auto after = idx; after != nodes.size(); ++after)
    {
        nodes[after]->idx_ = after;
        if (isDepot)  // then we need to decrease each following trip index
            nodes[after]->trip_--;
    }

#ifndef NDEBUG
    dirty = true;
#endif
}

void Route::swap(Node *first, Node *second)
{
    assert(!first->isDepot() && !second->isDepot());

    // TODO specialise std::swap for Node
    if (first->route_)
        first->route_->nodes[first->idx_] = second;

    if (second->route_)
        second->route_->nodes[second->idx_] = first;

    std::swap(first->route_, second->route_);
    std::swap(first->idx_, second->idx_);
    std::swap(first->trip_, second->trip_);

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
    for (auto const *node : nodes)
    {
        if (node->isDepot())
            continue;

        ProblemData::Client const &clientData = data.location(node->client());
        centroid_.first += static_cast<double>(clientData.x) / numClients();
        centroid_.second += static_cast<double>(clientData.y) / numClients();
    }

    // Distance.
    auto const &distMat = data.distanceMatrix(profile());

    cumDist.resize(nodes.size());
    cumDist[0] = 0;
    for (size_t idx = 1; idx != nodes.size(); ++idx)
        cumDist[idx] = cumDist[idx - 1] + distMat(visits[idx - 1], visits[idx]);

#ifndef PYVRP_NO_TIME_WINDOWS
    // Duration.
    durAt.resize(nodes.size());

    ProblemData::Depot const &start = data.location(startDepot());
    DurationSegment const vehStart(vehicleType_, vehicleType_.startLate);
    DurationSegment const depotStart(start);
    durAt[0] = DurationSegment::merge(0, vehStart, depotStart);

    ProblemData::Depot const &end = data.location(endDepot());
    DurationSegment const depotEnd(end);
    DurationSegment const vehEnd(vehicleType_, vehicleType_.twLate);
    durAt[nodes.size() - 1] = DurationSegment::merge(0, depotEnd, vehEnd);

    for (size_t idx = 1; idx != nodes.size() - 1; ++idx)
    {
        auto const *node = nodes[idx];

        if (!node->isReloadDepot())
        {
            ProblemData::Client const &client = data.location(node->client());
            durAt[idx] = {client};
        }
        else
        {
            ProblemData::Depot const &depot = data.location(node->client());
            durAt[idx] = {depot};
        }
    }

    auto const &durations = data.durationMatrix(profile());

    durBefore.resize(nodes.size());
    durBefore[0] = durAt[0];
    for (size_t idx = 1; idx != nodes.size(); ++idx)
    {
        auto const prev = idx - 1;
        auto const before = nodes[prev]->isReloadDepot()
                                ? durBefore[prev].finaliseBack()
                                : durBefore[prev];

        auto const edgeDur = durations(visits[prev], visits[idx]);
        durBefore[idx] = DurationSegment::merge(edgeDur, before, durAt[idx]);
    }

    durAfter.resize(nodes.size());
    durAfter[nodes.size() - 1] = durAt[nodes.size() - 1];
    for (size_t next = nodes.size() - 1; next != 0; --next)
    {
        auto const idx = next - 1;
        auto const after = nodes[next]->isReloadDepot()
                               ? durAfter[next].finaliseFront()
                               : durAfter[next];

        auto const edgeDur = durations(visits[idx], visits[next]);
        durAfter[idx] = DurationSegment::merge(edgeDur, durAt[idx], after);
    }
#endif

    // Load.
    for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
    {
        auto const capacity = vehicleType_.capacity[dim];

        loadAt[dim].resize(nodes.size());
        loadAt[dim][0] = {vehicleType_, dim};  // initial load
        loadAt[dim][nodes.size() - 1] = {};

        for (size_t idx = 1; idx != nodes.size() - 1; ++idx)
            loadAt[dim][idx]
                = nodes[idx]->isReloadDepot()
                      ? LoadSegment{}
                      : LoadSegment{data.location(visits[idx]), dim};

        loadBefore[dim].resize(nodes.size());
        loadBefore[dim][0] = loadAt[dim][0];
        for (size_t idx = 1; idx != nodes.size(); ++idx)
        {
            auto const prev = idx - 1;
            auto const before = nodes[prev]->isReloadDepot()
                                    ? loadBefore[dim][prev].finalise(capacity)
                                    : loadBefore[dim][prev];

            loadBefore[dim][idx] = LoadSegment::merge(before, loadAt[dim][idx]);
        }

        load_[dim] = 0;
        excessLoad_[dim]
            = loadBefore[dim][nodes.size() - 1].excessLoad(capacity);
        for (auto it = depots_.begin() + 1; it != depots_.end(); ++it)
            load_[dim] += loadBefore[dim][it->idx()].load();

        loadAfter[dim].resize(nodes.size());
        loadAfter[dim][nodes.size() - 1] = loadAt[dim][nodes.size() - 1];
        for (size_t idx = nodes.size() - 1; idx != 0; --idx)
        {
            auto const prev = idx - 1;
            auto const after = nodes[idx]->isReloadDepot()
                                   ? loadAfter[dim][idx].finalise(capacity)
                                   : loadAfter[dim][idx];

            loadAfter[dim][prev] = LoadSegment::merge(loadAt[dim][prev], after);
        }
    }

#ifndef NDEBUG
    dirty = false;
#endif
}

std::ostream &operator<<(std::ostream &out, pyvrp::search::Route const &route)
{
    for (size_t idx = 1; idx != route.size() - 1; ++idx)
    {
        if (idx != 1)
            out << ' ';

        if (route[idx]->isReloadDepot())
            out << '|';
        else
            out << *route[idx];
    }

    return out;
}

std::ostream &operator<<(std::ostream &out,
                         pyvrp::search::Route::Node const &node)
{
    return out << node.client();
}
