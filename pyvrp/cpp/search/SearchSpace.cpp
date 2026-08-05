#include "SearchSpace.h"

#include <cassert>
#include <sstream>
#include <stdexcept>

using pyvrp::Activity;
using pyvrp::search::Route;
using pyvrp::search::SearchSpace;

SearchSpace::SearchSpace(ProblemData const &data, Neighbourhood neighbours)
    : neighbours_(neighbours),
      promising_(data.numClients() + data.numShipments())
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

void SearchSpace::setNeighbours(Neighbourhood neighbours)
{
    neighbours_ = neighbours;
}

pyvrp::search::Neighbourhood const &SearchSpace::neighbours() const
{
    return neighbours_;
}

std::vector<Activity> const &
SearchSpace::neighboursOf(Activity const &activity) const
{
    return neighbours_[activity];
}

bool SearchSpace::isPromising(Activity const &activity) const
{
    assert(activity.isClient() || activity.isShipment());
    return activity.isClient()
               ? promising_[activity.idx()]
               : promising_[promising_.size() - activity.idx() - 1];
}

void SearchSpace::markPromising(Activity const &activity)
{
    assert(activity.isClient() || activity.isShipment());
    if (activity.isClient())
        promising_[activity.idx()] = true;
    else
        promising_[promising_.size() - activity.idx() - 1] = true;
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
