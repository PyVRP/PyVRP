#include "SearchSpace.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <utility>

using pyvrp::Activity;
using pyvrp::search::Route;
using pyvrp::search::SearchSpace;

SearchSpace::SearchSpace(ProblemData const &data, Neighbours neighbours)
    : promising_(data.numClients() + data.numShipments()),
      numClients_(data.numClients()),
      numShipments_(data.numShipments()),
      lastTest_(data.numClients() + data.numShipments()),
      lastUpdate_(data.numVehicles())
{
    if (neighbours.size() != data.numClients() + data.numShipments())
        throw std::runtime_error(
            "Neighbourhood dimension does not match problem dimension.");

    setNeighbours(neighbours);

    activityOrder_.reserve(data.numClients() + data.numShipments());
    for (size_t idx = 0; idx != data.numClients(); ++idx)
        activityOrder_.emplace_back(Activity::ActivityType::CLIENT, idx);
    for (size_t idx = 0; idx != data.numShipments(); ++idx)
        activityOrder_.emplace_back(Activity::ActivityType::PICKUP, idx);

    size_t offset = 0;
    for (size_t vehType = 0; vehType != data.numVehicleTypes(); vehType++)
    {
        vehTypeOrder_.emplace_back(vehType, offset);
        offset += data.vehicleType(vehType).numAvailable;
    }
}

void SearchSpace::reset(std::span<Route const> routes)
{
    assert(routes.size() == lastUpdate_.size());
    routes_ = routes;
    std::fill(lastTest_.begin(), lastTest_.end(), -1);
    std::fill(lastUpdate_.begin(), lastUpdate_.end(), 0);
    numUpdates_ = 0;
}

int SearchSpace::test(Activity const &activity)
{
    return std::exchange(lastTest_[index(activity)], numUpdates_);
}

bool SearchSpace::updatedSince(Route const *U,
                               Route const *V,
                               int lastTest) const
{
    auto const lastUpdate = [&](Route const *route)
    { return route ? lastUpdate_[route - routes_.data()] : 0; };
    return lastUpdate(U) > lastTest || lastUpdate(V) > lastTest;
}

void SearchSpace::update(Route const *U, Route const *V)
{
    ++numUpdates_;
    if (U)
        lastUpdate_[U - routes_.data()] = numUpdates_;
    if (V && U != V)
        lastUpdate_[V - routes_.data()] = numUpdates_;
}

size_t SearchSpace::numUpdates() const { return numUpdates_; }

void SearchSpace::touch(Activity const &activity)
{
    if (dynamic_)
        dynamic_->touch(activity);
}

void SearchSpace::touch(Route::Node const *U, Route::Node const *V)
{
    touch(U->activity());
    touch(V->activity());
}

void SearchSpace::setNeighbours(Neighbours neighbours)
{
    if (!neighbours_.empty() && neighbours.size() != neighbours_.size())
        throw std::runtime_error("Neighbourhood dimensions do not match.");

    for (auto const &[activity, neighbourhood] : neighbours)
    {
        if (!activity.isClient() && !activity.isPickup())
        {
            std::ostringstream msg;
            msg << "Expected neighbourhoods for clients and pickups, not "
                << activity << ".";
            throw std::runtime_error(msg.str());
        }

        auto const beginPos = neighbourhood.begin();
        auto const endPos = neighbourhood.end();

        auto const pred = [&](auto const &item) { return item == activity; };

        if (std::any_of(beginPos, endPos, pred))
        {
            std::ostringstream msg;
            msg << "Neighbourhood of " << activity << " contains itself.";
            throw std::runtime_error(msg.str());
        }
    }

    dynamic_.reset();
    neighbours_ = neighbours;
}

SearchSpace::Neighbours const &SearchSpace::neighbours() const
{
    return neighbours_;
}

std::vector<Activity> const &
SearchSpace::neighboursOf(Activity const &activity) const
{
    return neighbours_.at(activity);
}

size_t SearchSpace::index(Activity const &activity) const
{
    assert(activity.isClient() || activity.isShipment());
    return activity.isClient() ? activity.idx() : numClients_ + activity.idx();
}

bool SearchSpace::isPromising(Activity const &activity) const
{
    return promising_[index(activity)];
}

void SearchSpace::markPromising(Activity const &activity)
{
    touch(activity);
    promising_[index(activity)] = true;
}

void SearchSpace::markPromising(Route::Node const *node)
{
    assert(node->route());

    if (node->isClient() || node->isShipment())
        markPromising(node->activity());

    if (!node->isStartDepot() && (p(node)->isClient() || p(node)->isShipment()))
        markPromising(p(node)->activity());

    if (!node->isEndDepot() && (n(node)->isClient() || n(node)->isShipment()))
        markPromising(n(node)->activity());
}

void SearchSpace::markAllPromising() { promising_.set(); }

void SearchSpace::unmarkAllPromising() { promising_.reset(); }

std::vector<Activity> const &SearchSpace::activityOrder() const
{
    return activityOrder_;
}

std::vector<std::pair<size_t, size_t>> const &SearchSpace::vehTypeOrder() const
{
    return vehTypeOrder_;
}

void SearchSpace::shuffle(RandomNumberGenerator &rng)
{
    rng.shuffle(activityOrder_.begin(), activityOrder_.end());
    rng.shuffle(vehTypeOrder_.begin(), vehTypeOrder_.end());
}
