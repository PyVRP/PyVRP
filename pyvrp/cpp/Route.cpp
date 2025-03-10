#include "Route.h"
#include "DurationSegment.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <numeric>

using pyvrp::Cost;
using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Load;
using pyvrp::Route;

using Client = size_t;

Route::Iterator::Iterator(Route const &route, size_t idx)
    : route(&route), idx(idx)
{
    assert(idx <= route.size());
}

bool Route::Iterator::operator==(Iterator const &other) const
{
    return route == other.route && idx == other.idx;
}

Client Route::Iterator::operator*() const { return (*route)[idx]; }

Route::Iterator Route::Iterator::operator++(int)
{
    auto tmp = *this;
    ++*this;
    return tmp;
}

Route::Iterator &Route::Iterator::operator++()
{
    ++idx;
    return *this;
}

Route::ScheduledVisit::ScheduledVisit(Duration startService,
                                      Duration endService,
                                      Duration waitDuration,
                                      Duration timeWarp)
    : startService(startService),
      endService(endService),
      waitDuration(waitDuration),
      timeWarp(timeWarp)
{
    assert(startService <= endService);
}

Duration Route::ScheduledVisit::serviceDuration() const
{
    return endService - startService;
}

Route::Route(ProblemData const &data, Visits visits, size_t vehicleType)
    : Route(data,
            {{data,
              std::move(visits),
              vehicleType,
              data.vehicleType(vehicleType).startDepot,
              data.vehicleType(vehicleType).endDepot}},
            vehicleType)
{
}

Route::Route(ProblemData const &data, Trips trips, size_t vehType)
    : trips_(std::move(trips)),
      delivery_(data.numLoadDimensions(), 0),
      pickup_(data.numLoadDimensions(), 0),
      excessLoad_(data.numLoadDimensions(), 0),
      centroid_({0, 0}),
      vehicleType_(vehType)
{
    auto const &vehData = data.vehicleType(vehType);
    startDepot_ = vehData.startDepot;
    endDepot_ = vehData.endDepot;

    if (trips_.empty())  // then we insert a dummy trip for ease of validation.
        trips_.emplace_back(data, Visits{}, vehType, startDepot_, endDepot_);

    if (trips_[0].startDepot() != startDepot_)
    {
        auto const *msg = "Route must start at vehicle's start_depot.";
        throw std::invalid_argument(msg);
    }

    if (trips_.back().endDepot() != endDepot_)
        throw std::invalid_argument("Route must end at vehicle's end_depot.");

    for (size_t idx = 0; idx + 1 != trips_.size(); ++idx)
        if (trips_[idx].endDepot() != trips_[idx + 1].startDepot())
        {
            auto *msg = "Consecutive trips must end and start at same depot.";
            throw std::invalid_argument(msg);
        }

    auto const &durations = data.durationMatrix(vehData.profile);
    DurationSegment ds = {vehData, vehData.startLate};

    for (auto const &trip : trips_)
    {
        // TODO release time is per trip; not whole route
        ProblemData::Depot const &start = data.location(trip.startDepot());
        ds = DurationSegment::merge(0, ds, {start, start.serviceDuration});

        distance_ += trip.distance();
        service_ += trip.serviceDuration();
        travel_ += trip.travelDuration();
        prizes_ += trip.prizes();

        auto const [x, y] = trip.centroid();
        centroid_.first += (x * trip.size()) / size();
        centroid_.second += (y * trip.size()) / size();

        auto const &tripDelivery = trip.delivery();
        auto const &tripPickup = trip.pickup();
        auto const &tripExcessLoad = trip.excessLoad();
        for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
        {
            delivery_[dim] += tripDelivery[dim];
            pickup_[dim] += tripPickup[dim];
            excessLoad_[dim] += tripExcessLoad[dim];
        }

        size_t prevClient = trip.startDepot();
        for (auto const client : trip)
        {
            auto const edgeDuration = durations(prevClient, client);
            DurationSegment const clientDS = {data.location(client)};
            ds = DurationSegment::merge(edgeDuration, ds, clientDS);

            prevClient = client;
        }

        ProblemData::Depot const &end = data.location(trip.endDepot());
        ds = DurationSegment::merge(
            durations(prevClient, trip.endDepot()), ds, {end, 0});
    }

    distanceCost_ = vehData.unitDistanceCost * static_cast<Cost>(distance_);
    excessDistance_ = std::max<Distance>(distance_ - vehData.maxDistance, 0);

    ds = DurationSegment::merge(0, ds, {vehData, vehData.twLate});
    duration_ = ds.duration();
    durationCost_ = vehData.unitDurationCost * static_cast<Cost>(duration_);
    startTime_ = ds.twEarly();
    slack_ = ds.twLate() - ds.twEarly();
    timeWarp_ = ds.timeWarp(vehData.maxDuration);
    release_ = trips_[0].releaseTime();

    schedule_.reserve(size());
    auto now = startTime_;
    for (auto const &trip : trips_)
    {
        auto func = [&now](auto const &where)
        {
            auto const wait = std::max<Duration>(where.twEarly - now, 0);
            auto const tw = std::max<Duration>(now - where.twLate, 0);
            return std::make_pair(wait, tw);
        };

        ProblemData::Depot const &start = data.location(trip.startDepot());
        auto [wait, timeWarp] = func(start);

        now += wait;
        now -= timeWarp;

        now += start.serviceDuration;
        size_t prevClient = trip.startDepot();

        for (auto const client : trip)
        {
            now += durations(prevClient, client);

            ProblemData::Client const &clientData = data.location(client);
            auto [wait, timeWarp] = func(clientData);

            now += wait;
            now -= timeWarp;

            schedule_.emplace_back(
                now, now + clientData.serviceDuration, wait, timeWarp);

            now += clientData.serviceDuration;
            prevClient = client;
        }

        now += durations(prevClient, trip.endDepot());
    }
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
             Duration release,
             Duration startTime,
             Duration slack,
             Cost prizes,
             std::pair<double, double> centroid,
             size_t vehicleType,
             size_t startDepot,
             size_t endDepot,
             std::vector<ScheduledVisit> schedule)
    : trips_(std::move(trips)),
      schedule_(std::move(schedule)),
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

    // TODO this should probably become std::unreachable() once more compilers
    // support it.
    throw std::out_of_range("Index out of range.");
}

Route::Iterator Route::begin() const { return Iterator(*this, 0); }

Route::Iterator Route::end() const { return Iterator(*this, size()); }

Route::Trips const &Route::trips() const { return trips_; }

Route::Visits Route::visits() const { return {begin(), end()}; }

std::vector<Route::ScheduledVisit> const &Route::schedule() const
{
    return schedule_;
}

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
    auto const &trips = route.trips();
    for (size_t idx = 0; idx != trips.size(); ++idx)
    {
        out << trips[idx];
        if (idx + 1 != trips.size())
            out << " | ";
    }

    return out;
}
