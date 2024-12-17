#include "Route.h"

#include <cmath>
#include <numbers>
#include <ostream>
#include <utility>

using pyvrp::search::Route;

Route::Node::Node(size_t loc, NodeType type)
    : loc_(loc), idx_(0), tripIdx_(0), route_(nullptr), type_(type)
{
}

void Route::Node::assign(Route *route, size_t idx, size_t tripIdx)
{
    assert(!route_);  // must not currently be assigned
    idx_ = idx;
    tripIdx_ = tripIdx;
    route_ = route;
}

void Route::Node::unassign()
{
    idx_ = 0;
    tripIdx_ = 0;
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

    // Clear nodes.
    nodes.clear();
    depotNodes.clear();

    // We should never reallocate this vector which would invalidate the node
    // pointers of the depots! Therefore, enough space must be reserved which is
    // the maximum number of trips plus one, such that there can be
    // (temporarily) one extra trip in the route when applying a SwapTails move.
    depotNodes.reserve(vehicleType_.maxTrips + 1);

    // Insert depot nodes of the first trip.
    addTrip();

    update();
}

void Route::insert(size_t idx, Node *node)
{
    assert(0 < idx && idx < nodes.size());
    assert(!node->isDepot());
    // Not allowed to insert between depot unload and load node.
    assert(!nodes[idx - 1]->isDepotUnload());

    // Note that trip indices do not change when only inserting clients.
    size_t tripIdx = nodes[idx - 1]->tripIdx();
    node->assign(this, idx, tripIdx);
    nodes.insert(nodes.begin() + idx, node);

    for (size_t after = idx + 1; after != nodes.size(); ++after)
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

void Route::insertTrip(size_t idx)
{
    assert(idx <= nodes.size());
    // Not allowed to insert new trip in the middle of an other trip.
    assert(idx == 0 || nodes[idx - 1]->isDepotUnload());

    size_t const trip
        = idx == nodes.size() ? numTrips() : nodes[idx]->tripIdx();
    auto depotPair = depotNodes.emplace(
        depotNodes.begin() + trip,
        Node(vehicleType_.startDepot, Node::NodeType::DepotLoad),
        Node(vehicleType_.endDepot, Node::NodeType::DepotUnload));

    auto &[tripStartDepot, tripEndDepot] = *depotPair;
    tripStartDepot.assign(this, idx, trip);
    tripEndDepot.assign(this, idx + 1, trip);

    nodes.insert(nodes.begin() + idx, &tripStartDepot);
    nodes.insert(nodes.begin() + idx + 1, &tripEndDepot);

    // Update depot node pointers.
    depotPair++;
    for (size_t after = idx + 2; after != nodes.size(); ++after)
    {
        if (nodes[after]->isDepotLoad())
            nodes[after] = &depotPair->first;
        else if (nodes[after]->isDepotUnload())
        {
            nodes[after] = &depotPair->second;
            depotPair++;
        }

        nodes[after]->idx_ = after;
        nodes[after]->tripIdx_++;
    }

#ifndef NDEBUG
    dirty = true;
#endif
}

void Route::addTrip()
{
    insertTrip(nodes.size());

#ifndef NDEBUG
    dirty = true;
#endif
}

void Route::remove(size_t idx)
{
    assert(0 < idx && idx < nodes.size() - 1);
    assert(nodes[idx]->route() == this);  // must currently be in this route
    assert(!nodes[idx]->isDepot());  // otherwise trip indices must be updated

    nodes[idx]->unassign();
    nodes.erase(nodes.begin() + idx);

    for (auto after = idx; after != nodes.size(); ++after)
        nodes[after]->idx_ = after;

#ifndef NDEBUG
    dirty = true;
#endif
}

void Route::removeTrip(size_t tripIdx)
{
    assert(tripIdx < numTrips());
    assert(numTrips() > 1);

    auto &[depotLoadNode, depotUnloadNode] = depotNodes[tripIdx];

    assert(depotLoadNode.tripIdx() == tripIdx);
    assert(depotUnloadNode.tripIdx() == tripIdx);
    assert(depotLoadNode.route() == this);
    assert(depotUnloadNode.route() == this);

    size_t const startIdx = depotLoadNode.idx();
    size_t const endIdx = depotUnloadNode.idx();
    assert(startIdx + 1 == endIdx);  // Trip must be empty.

    nodes.erase(nodes.begin() + endIdx);    // Removes depot unload node.
    nodes.erase(nodes.begin() + startIdx);  // Removes depot load node.

    auto depotPair = depotNodes.erase(depotNodes.begin() + tripIdx);

    // Update depot node pointers.
    for (auto after = startIdx; after != nodes.size(); ++after)
    {
        if (nodes[after]->isDepotLoad())
            nodes[after] = &depotPair->first;
        else if (nodes[after]->isDepotUnload())
        {
            nodes[after] = &depotPair->second;
            depotPair++;
        }

        nodes[after]->idx_ = after;
        nodes[after]->tripIdx_--;
    }

#ifndef NDEBUG
    dirty = true;
#endif
}

void Route::swap(Node *first, Node *second)
{
    // TODO specialise std::swap for Node
    assert(!first->isDepot());
    assert(!second->isDepot());

    if (first->route_)
        first->route_->nodes[first->idx_] = second;

    if (second->route_)
        second->route_->nodes[second->idx_] = first;

    std::swap(first->route_, second->route_);
    std::swap(first->idx_, second->idx_);
    std::swap(first->tripIdx_, second->tripIdx_);

#ifndef NDEBUG
    if (first->route_)
        first->route_->dirty = true;

    if (second->route_)
        second->route_->dirty = true;
#endif
}

void Route::update()
{
    assert(depotNodes.size() >= 1);
    assert(nodes.size() >= 2);  // Depot nodes of first trip must be included.

    size_t numTrips = this->numTrips();
    assert(numTrips == nodes.back()->tripIdx() + 1);
    assert(numTrips <= vehicleType_.maxTrips);

    std::vector<size_t> tripStart;
    tripStart.reserve(numTrips);
    std::vector<size_t> tripEnd;
    tripEnd.reserve(numTrips);

    size_t numClients = 0;
    visits.clear();

    size_t depotBalance = 0;
    for (size_t idx = 0; idx != nodes.size(); ++idx)
    {
        auto const *node = nodes[idx];
        visits.emplace_back(node->client());

        if (node->isClient())
        {
            assert(depotBalance == 1);
            ++numClients;
        }
        else if (node->isDepotLoad())
        {
            assert(depotBalance == 0);
            depotBalance++;
            tripStart.push_back(idx);
        }
        else  // Depot unload
        {
            assert(node->isDepotUnload());
            assert(depotBalance == 1);
            depotBalance--;
            tripEnd.push_back(idx);
            assert(tripStart.size() == tripEnd.size());
        }
    }

    assert(numClients == this->numClients());

    // Assert no empty trips, unless there is one single empty trip.
    if (numTrips > 1)
        for (size_t trip = 0; trip != numTrips; ++trip)
            assert(tripEnd[trip] > tripStart[trip] + 1);

    centroid_ = {0, 0};
    for (size_t idx = 1; idx != nodes.size() - 1; ++idx)
    {
        if (nodes[idx]->isDepot())
            continue;

        ProblemData::Client const &clientData = data.location(visits[idx]);
        centroid_.first += static_cast<double>(clientData.x) / numClients;
        centroid_.second += static_cast<double>(clientData.y) / numClients;
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
    durAt[0] = {vehicleType_, vehicleType_.startLate};
    for (size_t idx = 1; idx != nodes.size(); ++idx)
        durAt[idx] = nodes[idx]->isClient()
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
        tripLoadBefore[dim].resize(nodes.size());
        tripLoad_[dim].resize(numTrips);
        excessLoad_[dim] = 0;
        tripLoadAfter[dim].resize(nodes.size());

        for (size_t trip = 0; trip != numTrips; ++trip)
        {
            // LoadAt
            loadAt[dim][tripStart[trip]] = LoadSegment();
            for (size_t idx = tripStart[trip] + 1; idx != tripEnd[trip]; ++idx)
                loadAt[dim][idx] = LoadSegment(data.location(visits[idx]), dim);

            loadAt[dim][tripEnd[trip]] = LoadSegment();

            // TripLoadBefore
            tripLoadBefore[dim][tripStart[trip]] = loadAt[dim][tripStart[trip]];
            for (size_t idx = tripStart[trip]; idx != tripEnd[trip]; ++idx)
                tripLoadBefore[dim][idx + 1] = LoadSegment::merge(
                    tripLoadBefore[dim][idx], loadAt[dim][idx + 1]);

            // TripLoad and ExcessLoad
            tripLoad_[dim][trip] = tripLoadBefore[dim][tripEnd[trip]];
            excessLoad_[dim] += std::max<Load>(
                tripLoad_[dim][trip].load() - capacity()[dim], 0);

            // TripLoadAfter
            tripLoadAfter[dim][tripEnd[trip]] = loadAt[dim][tripEnd[trip]];
            for (size_t idx = tripEnd[trip]; idx != tripStart[trip]; --idx)
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
        if (node->isClient())
            out << ' ' << node->client();  // client index
        else if (node->isDepotLoad() && node != *route.begin())
            out << " |";  // trip delimiter
    }
    out << '\n';

    return out;
}
