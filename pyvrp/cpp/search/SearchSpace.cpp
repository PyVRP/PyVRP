#include "SearchSpace.h"

#include <cassert>
#include <sstream>
#include <stdexcept>

using pyvrp::Activity;
using pyvrp::search::Route;
using pyvrp::search::SearchSpace;

SearchSpace::SearchSpace(ProblemData const &data, Neighbours neighbours)
    : promising_(data.numClients())
{
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

void SearchSpace::setNeighbours(Neighbours neighbours)
{
    if (neighbours.size() != neighbours_.size())
        throw std::runtime_error("Neighbourhood dimensions do not match.");

    for (auto const &[activity, neighbourhood] : neighbours)
    {
        auto const beginPos = neighbourhood.begin();
        auto const endPos = neighbourhood.end();

        auto const pred = [&](auto item) { return item == activity; };

        if (std::any_of(beginPos, endPos, pred))
        {
            std::ostringstream msg;
            msg << "Neighbourhood of " << activity << " contains itself.";
            throw std::runtime_error(msg.str());
        }
    }

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

bool SearchSpace::isPromising(Activity const &activity) const
{
    assert(activity.isClient() || activity.isShipment());
    return false;  // TODO
}

void SearchSpace::markPromising([[maybe_unused]] Activity const &activity)
{
    // promising_[activity] = true;  TODO
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
