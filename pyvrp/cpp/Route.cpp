#include "Route.h"
#include "DurationSegment.h"
#include "LoadSegment.h"

#include <algorithm>
#include <fstream>
#include <numeric>

using pyvrp::Cost;
using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Load;
using pyvrp::Route;

using Client = size_t;

Route::Iterator::Iterator(Trips const &trips, size_t trip, size_t visit)
    : trips(&trips), trip(trip), visit(visit)
{
}

Route::Iterator Route::Iterator::begin(Trips const &trips)
{
    // Skip empty trips.
    for (size_t trip = 0; trip != trips.size(); ++trip)
        if (!trips[trip].empty())
            return Iterator(trips, trip, 0);

    return Iterator(trips, trips.size(), 0);
}

Route::Iterator Route::Iterator::end(Trips const &trips)
{
    return Iterator(trips, trips.size(), 0);
}

bool Route::Iterator::operator==(Iterator const &other) const
{
    return trips == other.trips && trip == other.trip && visit == other.visit;
}

Client Route::Iterator::operator*() const
{
    assert(trip < trips->size());
    assert(visit < (*trips)[trip].size());
    return (*trips)[trip][visit];
}

Route::Iterator Route::Iterator::operator++(int)
{
    auto tmp = *this;
    ++*this;
    return tmp;
}

Route::Iterator &Route::Iterator::operator++()
{
    auto const tripSize = (*trips)[trip].size();

    if (visit + 1 >= tripSize)
    {
        // Skip empty trips
        while (trip + 1 < trips->size() && (*trips)[trip + 1].empty())
            ++trip;

        ++trip;
        visit = 0;
    }
    else
        ++visit;

    return *this;
}

Route::Route(ProblemData const &data, Trip visits, size_t const vehicleType)
    : Route(data, std::vector<Trip>{visits}, vehicleType)
{
}

Route::Route(ProblemData const &data, Trips visits, size_t const vehicleType)
    : trips_(std::move(visits)), centroid_({0, 0}), vehicleType_(vehicleType)
{
    auto const &vehType = data.vehicleType(vehicleType);
    startDepot_ = vehType.startDepot;
    endDepot_ = vehType.endDepot;

    auto const &distances = data.distanceMatrix(vehType.profile);
    auto const &durations = data.durationMatrix(vehType.profile);

    delivery_ = std::vector<Load>(data.numLoadDimensions(), 0);
    pickup_ = std::vector<Load>(data.numLoadDimensions(), 0);
    excessLoad_ = std::vector<Load>(data.numLoadDimensions(), 0);

    DurationSegment routeDs = {vehType, vehType.startLate};
    for (size_t trip = 0; trip != trips_.size(); ++trip)
    {
        size_t prev = trip == 0 ? startDepot_ : endDepot_;
        Duration const depotTwLate
            = trip == 0 ? vehType.startLate : vehType.twLate;

        std::vector<LoadSegment> loadSegments(data.numLoadDimensions());
        DurationSegment ds = {vehType, depotTwLate};

        for (auto const client : trips_[trip])
        {
            ProblemData::Client const &clientData = data.location(client);

            distance_ += distances(prev, client);
            travel_ += durations(prev, client);
            service_ += clientData.serviceDuration;
            prizes_ += clientData.prize;

            centroid_.first += static_cast<double>(clientData.x) / size();
            centroid_.second += static_cast<double>(clientData.y) / size();

            auto const clientDS = DurationSegment(clientData);
            ds = DurationSegment::merge(durations(prev, client), ds, clientDS);

            for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
            {
                auto const clientLs = LoadSegment(clientData, dim);
                loadSegments[dim]
                    = LoadSegment::merge(loadSegments[dim], clientLs);
            }

            prev = client;
        }

        size_t end = endDepot_;  // Trips always end at the end depot.
        distance_ += distances(prev, end);
        distanceCost_ = vehType.unitDistanceCost * static_cast<Cost>(distance_);
        excessDistance_
            = std::max<Distance>(distance_ - vehType.maxDistance, 0);

        travel_ += durations(prev, end);

        for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
        {
            delivery_[dim] += loadSegments[dim].delivery();
            pickup_[dim] += loadSegments[dim].pickup();
            excessLoad_[dim] += std::max<Load>(
                loadSegments[dim].load() - vehType.capacity[dim], 0);
        }

        DurationSegment endDS(vehType, vehType.twLate);
        ds = DurationSegment::merge(durations(prev, endDepot_), ds, endDS);

        // No edge duration between trips.
        routeDs = DurationSegment::merge(0, routeDs, ds);
    }

    duration_ = routeDs.duration();
    durationCost_ = vehType.unitDurationCost * static_cast<Cost>(duration_);
    startTime_ = routeDs.twEarly();
    slack_ = routeDs.twLate() - routeDs.twEarly();
    timeWarp_ = routeDs.timeWarp(vehType.maxDuration);
    release_ = routeDs.releaseTime();
}

Route::Route(Trips trips,
             Distance distance,
             Cost distanceCost,
             Distance excessDistance,
             std::vector<Load> delivery,
             std::vector<Load> pickup,
             std::vector<Load> excessLoad,
             Duration duration,
             Cost durationCost,
             Duration timeWarp,
             Duration travel,
             Duration service,
             Duration wait,
             Duration release,
             Duration startTime,
             Duration slack,
             Cost prizes,
             std::pair<double, double> centroid,
             size_t vehicleType,
             size_t startDepot,
             size_t endDepot)
    : trips_(std::move(trips)),
      distance_(distance),
      distanceCost_(distanceCost),
      excessDistance_(excessDistance),
      delivery_(std::move(delivery)),
      pickup_(std::move(pickup)),
      excessLoad_(std::move(excessLoad)),
      duration_(duration),
      durationCost_(durationCost),
      timeWarp_(timeWarp),
      travel_(travel),
      service_(service),
      wait_(wait),
      release_(release),
      startTime_(startTime),
      slack_(slack),
      prizes_(prizes),
      centroid_(centroid),
      vehicleType_(vehicleType),
      startDepot_(startDepot),
      endDepot_(endDepot)
{
}

bool Route::empty() const { return size() == 0; }

size_t Route::size() const
{
    return std::accumulate(trips_.begin(),
                           trips_.end(),
                           0,
                           [](size_t count, auto const &trip)
                           { return count + trip.size(); });
}

Client Route::operator[](size_t idx) const
{
    for (auto const &trip : trips_)
        if (idx < trip.size())
            return trip[idx];
        else
            idx -= trip.size();

    throw std::out_of_range("Index out of range.");
}

Route::Iterator Route::begin() const { return Iterator::begin(trips_); }

Route::Iterator Route::end() const { return Iterator::end(trips_); }

std::vector<Client> const Route::visits() const { return {begin(), end()}; }

Route::Trips const &Route::trips() const { return trips_; }

std::vector<Client> const &Route::trip(size_t trip) const
{
    assert(trip < trips_.size());
    return trips_[trip];
}

size_t Route::numTrips() const { return trips_.size(); }

Distance Route::distance() const { return distance_; }

Cost Route::distanceCost() const { return distanceCost_; }

Distance Route::excessDistance() const { return excessDistance_; }

std::vector<Load> const &Route::delivery() const { return delivery_; }

std::vector<Load> const &Route::pickup() const { return pickup_; }

std::vector<Load> const &Route::excessLoad() const { return excessLoad_; }

Duration Route::duration() const { return duration_; }

Cost Route::durationCost() const { return durationCost_; }

Duration Route::serviceDuration() const { return service_; }

Duration Route::timeWarp() const { return timeWarp_; }

Duration Route::waitDuration() const { return duration_ - travel_ - service_; }

Duration Route::travelDuration() const { return travel_; }

Duration Route::startTime() const { return startTime_; }

Duration Route::endTime() const { return startTime_ + duration_ - timeWarp_; }

Duration Route::slack() const { return slack_; }

Duration Route::releaseTime() const { return release_; }

Cost Route::prizes() const { return prizes_; }

std::pair<double, double> const &Route::centroid() const { return centroid_; }

size_t Route::vehicleType() const { return vehicleType_; }

size_t Route::startDepot() const { return startDepot_; }

size_t Route::endDepot() const { return endDepot_; }

bool Route::isFeasible() const
{
    return !hasExcessLoad() && !hasTimeWarp() && !hasExcessDistance();
}

bool Route::hasExcessLoad() const
{
    return std::any_of(excessLoad_.begin(),
                       excessLoad_.end(),
                       [](auto const excess) { return excess > 0; });
}

bool Route::hasExcessDistance() const { return excessDistance_ > 0; }

bool Route::hasTimeWarp() const { return timeWarp_ > 0; }

bool Route::operator==(Route const &other) const
{
    // First compare simple attributes, since that's a quick and cheap check.
    // Only when these are the same we test if the visits are all equal.
    // clang-format off
    return distance_ == other.distance_
        && duration_ == other.duration_
        && timeWarp_ == other.timeWarp_
        && vehicleType_ == other.vehicleType_
        && trips_ == other.trips_;
    // clang-format on
}

std::ostream &operator<<(std::ostream &out, Route const &route)
{
    for (size_t i = 0; i != route.numTrips(); ++i)
    {
        for (auto const client : route.trip(i))
            out << client << ' ';

        if (i != route.numTrips() - 1)
            out << " | ";
    }

    return out;
}
