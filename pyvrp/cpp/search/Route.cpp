#include "Route.h"

#include <ostream>
#include <utility>

using pyvrp::search::Route;

Route::Node::Node(Activity::ActivityType type, size_t idx)
    : Node(Activity{type, idx})
{
}

Route::Node::Node(Activity activity)
    : activity_(activity), pos_(0), trip_(0), route_(nullptr)
{
}

void Route::Node::assign(Route *route, size_t pos, size_t trip)
{
    pos_ = pos;
    trip_ = trip;
    route_ = route;
}

void Route::Node::unassign()
{
    pos_ = 0;
    trip_ = 0;
    route_ = nullptr;
}

Route::Route(ProblemData const &data, size_t vehicleType)
    : data(data),
      vehicleType_(data.vehicleType(vehicleType)),
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
    return nodes.begin();
}

std::vector<Route::Node *>::const_iterator Route::end() const
{
    return nodes.end();
}

size_t Route::vehicleType() const
{
    auto const &vehicleTypes = data.vehicleTypes();
    return std::distance(&vehicleTypes[0], &vehicleType_);
}

void Route::clear()
{
    if (nodes.size() == 2)  // then the route is already empty and we have
        return;             // nothing to do.

    for (auto *node : nodes)        // only unassign if in route; node may not
        if (node->route() == this)  // be if it's been assigned to another route
            node->unassign();       // while loading a new solution into the LS

    nodes.clear();
    depots_.clear();

    depots_.emplace_back(Activity::ActivityType::DEPOT,
                         vehicleType_.startDepot);
    depots_.emplace_back(Activity::ActivityType::DEPOT, vehicleType_.endDepot);

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
    auto const isDepot = node->isDepot();

    if (isDepot)  // is depot, so we need to insert a copy into our own memory
    {
        if (depots_.size() == depots_.capacity())  // then we reallocate and
        {                                          // must update references
            depots_.reserve(depots_.size() + 1);
            for (auto &depot : depots_)
                nodes[depot.pos()] = &depot;
        }

        node = &depots_.emplace_back(node->activity());
    }

    if (numTrips() > maxTrips())
        throw std::invalid_argument("Vehicle cannot perform this many trips.");

    nodes.insert(nodes.begin() + idx, node);
    node->assign(this, idx, nodes[idx - 1]->trip());

    for (size_t after = idx; after != nodes.size(); ++after)
    {
        nodes[after]->pos_ = after;
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
            nodes[it->pos()] = &*it;
    }
    else
        // We do not own this node, so we only unassign it.
        nodes[idx]->unassign();

    nodes.erase(nodes.begin() + idx);  // remove dangling pointer
    for (auto after = idx; after != nodes.size(); ++after)
    {
        nodes[after]->pos_ = after;
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
        first->route_->nodes[first->pos_] = second;

    if (second->route_)
        second->route_->nodes[second->pos_] = first;

    std::swap(first->route_, second->route_);
    std::swap(first->pos_, second->pos_);
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
    locations.clear();
    for (auto const *node : nodes)
    {
        assert(node->isDepot() || node->isClient());

        if (node->isDepot())
            locations.emplace_back(data.depot(node->idx()).location);
        else
            locations.emplace_back(data.client(node->idx()).location);
    }

    // Distance.
    auto const &distMat = data.distanceMatrix(profile());

    cumDist.resize(nodes.size());
    cumDist[0] = 0;
    for (size_t idx = 1; idx != nodes.size(); ++idx)
        cumDist[idx]
            = cumDist[idx - 1] + distMat(locations[idx - 1], locations[idx]);

    // Duration.
    durAt.resize(nodes.size());

    auto const &start = data.depot(startDepot());
    DurationSegment const vehStart(vehicleType_, vehicleType_.startLate);
    DurationSegment const depotStart(start, start.serviceDuration);
    durAt[0] = DurationSegment::merge(vehStart, depotStart);

    auto const &end = data.depot(endDepot());
    DurationSegment const depotEnd(end, 0);
    DurationSegment const vehEnd(vehicleType_, vehicleType_.twLate);
    durAt[nodes.size() - 1] = DurationSegment::merge(depotEnd, vehEnd);

    for (size_t idx = 1; idx != nodes.size() - 1; ++idx)
    {
        auto const *node = nodes[idx];

        if (!node->isReloadDepot())
            durAt[idx] = {data.client(node->idx())};
        else
            durAt[idx] = {data.depot(node->idx()), 0};
    }

    auto const &durations = data.durationMatrix(profile());

    durBefore.resize(nodes.size());
    durBefore[0] = durAt[0];
    for (size_t idx = 1; idx != nodes.size(); ++idx)
    {
        auto const prev = idx - 1;
        auto before = nodes[prev]->isReloadDepot()
                          ? durBefore[prev].finaliseBack()
                          : durBefore[prev];

        if (nodes[prev]->isReloadDepot())
        {
            // Then we need to first account for depot service before we merge
            // with the idx segment.
            auto const &depot = data.depot(nodes[prev]->idx());
            before = DurationSegment::merge(before, {depot.serviceDuration});
        }

        auto const edgeDur = durations(locations[prev], locations[idx]);
        durBefore[idx] = DurationSegment::merge(edgeDur, before, durAt[idx]);
    }

    durAfter.resize(nodes.size());
    durAfter[nodes.size() - 1] = durAt[nodes.size() - 1];
    for (size_t next = nodes.size() - 1; next != 0; --next)
    {
        auto const idx = next - 1;
        auto after = nodes[next]->isReloadDepot()
                         ? durAfter[next].finaliseFront()
                         : durAfter[next];

        if (nodes[idx]->isReloadDepot())
        {
            // This is not entirely correct logically, since we now do service
            // at idx after already travelling to next, but that's OK since
            // we're essentially using the trick of adding service to the
            // outgoing edge.
            auto const &depot = data.depot(nodes[idx]->idx());
            after = DurationSegment::merge({depot.serviceDuration}, after);
        }

        auto const edgeDur = durations(locations[idx], locations[next]);
        durAfter[idx] = DurationSegment::merge(edgeDur, durAt[idx], after);
    }

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
                      : LoadSegment{data.client(nodes[idx]->idx()), dim};

        loadBefore[dim].resize(nodes.size());
        loadBefore[dim][0] = loadAt[dim][0];
        for (size_t idx = 1; idx != nodes.size(); ++idx)
        {
            auto const prev = idx - 1;
            if (nodes[prev]->isReloadDepot())
                loadBefore[dim][idx] = LoadSegment::merge(
                    loadBefore[dim][prev].finalise(capacity), loadAt[dim][idx]);
            else
                loadBefore[dim][idx] = LoadSegment::merge(loadBefore[dim][prev],
                                                          loadAt[dim][idx]);
        }

        load_[dim] = 0;
        excessLoad_[dim]
            = loadBefore[dim][nodes.size() - 1].excessLoad(capacity);
        for (auto it = depots_.begin() + 1; it != depots_.end(); ++it)
            load_[dim] += loadBefore[dim][it->pos()].load();

        loadAfter[dim].resize(nodes.size());
        loadAfter[dim][nodes.size() - 1] = loadAt[dim][nodes.size() - 1];
        for (size_t idx = nodes.size() - 1; idx != 0; --idx)
        {
            auto const prev = idx - 1;
            if (nodes[idx]->isReloadDepot())
                loadAfter[dim][prev] = LoadSegment::merge(
                    loadAt[dim][prev], loadAfter[dim][idx].finalise(capacity));
            else
                loadAfter[dim][prev] = LoadSegment::merge(loadAt[dim][prev],
                                                          loadAfter[dim][idx]);
        }
    }

    // These cost components are separately cached as well because they are
    // requested *a lot*.
    distance_ = cumDist.back();
    excessDistance_ = std::max<Distance>(distance_ - maxDistance(), 0);
    distanceCost_ = unitDistanceCost() * static_cast<Cost>(distance_);

    duration_ = durAfter[0].duration();
    timeWarp_ = durAfter[0].timeWarp(maxDuration());

    auto const overtime = std::max<Duration>(duration_ - shiftDuration(), 0);
    durationCost_ = unitDurationCost() * static_cast<Cost>(duration_)
                    + unitOvertimeCost() * static_cast<Cost>(overtime);

#ifndef NDEBUG
    dirty = false;
#endif
}

bool Route::operator==(Route const &other) const
{
    assert(!dirty && !other.dirty);

    // First compare simple attributes, since that's a quick and cheap check.
    // Only when these are the same we test if the nodes are all equal.
    // clang-format off
    return distance_ == other.distance_
        && duration_ == other.duration_
        && timeWarp_ == other.timeWarp_
        && vehicleType_ == other.vehicleType_
        && nodes == other.nodes;
    // clang-format on
}

std::ostream &operator<<(std::ostream &out, Route const &route)
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

std::ostream &operator<<(std::ostream &out, Route::Node const &node)
{
    return out << node.activity();
}

template <>
pyvrp::Cost
pyvrp::CostEvaluator::penalisedCost(pyvrp::search::Route const &route) const
{
    if (route.empty())
        return 0;

    // clang-format off
    return route.distanceCost()
         + route.durationCost()
         + route.fixedVehicleCost()
         + excessLoadPenalties(route.excessLoad())
         + twPenalty(route.timeWarp())
         + distPenalty(route.excessDistance(), 0);
    // clang-format on
}
