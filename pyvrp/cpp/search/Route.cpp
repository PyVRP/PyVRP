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
      startDepot_(vehicleType_.startDepot),
      endDepot_(vehicleType_.endDepot),
      distCache(data.numProfiles()),
      durCache(data.numProfiles()),
      loadCache(data.numLoadDimensions()),
      load_(data.numLoadDimensions(), 0),
      excessLoad_(data.numLoadDimensions(), 0)
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
    for (auto *node : nodes)  // unassign all nodes from route.
    {
        node->idx_ = 0;
        node->route_ = nullptr;
    }

    nodes.clear();  // clear nodes and reinsert the depots.
    nodes.push_back(&startDepot_);
    nodes.push_back(&endDepot_);

    startDepot_.idx_ = 0;
    startDepot_.route_ = this;

    endDepot_.idx_ = 1;
    endDepot_.route_ = this;

    update();

#ifndef NDEBUG
    dirty = false;
#endif
}

void Route::insert(size_t idx, Node *node)
{
    assert(0 < idx && idx < nodes.size());
    assert(!node->route());  // must previously have been unassigned

    node->idx_ = idx;
    node->route_ = this;
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

    auto *node = nodes[idx];

    node->idx_ = 0;
    node->route_ = nullptr;

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
    std::swap(first->route_->nodes[first->idx_],
              second->route_->nodes[second->idx_]);

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

    for (size_t idx = 1; idx != nodes.size() - 1; ++idx)
    {
        auto const *node = nodes[idx];
        assert(!node->isDepot());

        ProblemData::Client const &clientData = data.location(node->client());
        centroid_.first += static_cast<double>(clientData.x) / size();
        centroid_.second += static_cast<double>(clientData.y) / size();
    }

    // Set-up node concatenation schemes for distance and duration profiles.
    for (size_t profile = 0; profile != data.numProfiles(); ++profile)
    {
        auto &dists = distCache[profile];
        dists.resize(nodes.size(), nodes.size());
        std::fill(dists.begin(), dists.end(), std::nullopt);

        for (size_t idx = 0; idx != nodes.size(); ++idx)
            dists(idx, idx) = {nodes[idx]->client()};

        auto &durs = durCache[profile];
        durs.resize(nodes.size(), nodes.size());
        std::fill(durs.begin(), durs.end(), std::nullopt);

        durs(0, 0) = {vehicleType_.startDepot, vehicleType_};
        durs(nodes.size() - 1, nodes.size() - 1)
            = {vehicleType_.endDepot, vehicleType_};

        for (size_t idx = 1; idx != nodes.size() - 1; ++idx)
        {
            auto const *node = nodes[idx];
            auto const client = node->client();
            durs(idx, idx) = {client, data.location(client)};
        }
    }

    // Set-up node concatenation schemes for load dimensions.
    for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
    {
        auto &loads = loadCache[dim];
        loads.resize(nodes.size(), nodes.size());
        std::fill(loads.begin(), loads.end(), std::nullopt);

        loads(0, 0) = {0, 0, 0};
        loads(nodes.size() - 1, nodes.size() - 1) = {0, 0, 0};

        for (size_t idx = 1; idx != nodes.size() - 1; ++idx)
        {
            auto const *node = nodes[idx];
            auto const client = node->client();
            loads(idx, idx) = {data.location(client), dim};

            // We also compute the load prefix segments from the depot, since we
            // need this for the cached load and excess load values below.
            loads(0, idx) = LoadSegment::merge(loads(0, idx - 1).value(),
                                               loads(idx, idx).value());
        }

        assert(loads(0, nodes.size() - 2));
        load_[dim] = loads(0, nodes.size() - 2).value().load();
        excessLoad_[dim] = std::max<Load>(load_[dim] - capacity()[dim], 0);
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
