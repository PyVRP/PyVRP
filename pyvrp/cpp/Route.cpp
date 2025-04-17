#include "Route.h"
#include "DurationSegment.h"
#include "LoadSegment.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <numeric>

using pyvrp::Cost;
using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Load;
using pyvrp::Route;
using pyvrp::Trip;

using Client = size_t;

Route::Iterator::Iterator(Route const &route, size_t idx)
    : route_(&route), trip_(route.numTrips()), idx_(0)
{
    assert(idx <= route.size());

    auto const &trips = route.trips();
    for (size_t trip = 0; trip != trips.size(); ++trip)
    {
        if (idx < trips[trip].size())
        {
            trip_ = trip;
            idx_ = idx;
            break;
        }

        idx -= trips[trip].size();
    }
}

bool Route::Iterator::operator==(Iterator const &other) const
{
    return route_ == other.route_ && trip_ == other.trip_ && idx_ == other.idx_;
}

Client Route::Iterator::operator*() const
{
    auto const &trips = route_->trips();
    assert(trip_ < trips.size());
    assert(idx_ < trips[trip_].size());

    return trips[trip_][idx_];
}

Route::Iterator Route::Iterator::operator++(int)
{
    auto tmp = *this;
    ++*this;
    return tmp;
}

Route::Iterator &Route::Iterator::operator++()
{
    auto const &trips = route_->trips();
    if (idx_ + 1 < trips[trip_].size())
    {
        ++idx_;
        return *this;
    }

    // Then we move to the next trip. This trip could be empty - in that case
    // we continue to the next until we either exhaust all trips, or we find a
    // non-empty trip.
    ++trip_;
    while (trip_ + 1 < trips.size() && trips[trip_].empty())
        ++trip_;

    idx_ = 0;
    return *this;
}

Route::ScheduledVisit::ScheduledVisit(size_t location,
                                      size_t trip,
                                      Duration startService,
                                      Duration endService,
                                      Duration waitDuration,
                                      Duration timeWarp)
    : location(location),
      trip(trip),
      startService(startService),
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

void Route::validate(ProblemData const &data) const
{
    auto const &vehData = data.vehicleType(vehicleType_);

    if (trips_.size() > vehData.maxTrips())
        throw std::invalid_argument("Vehicle cannot perform this many trips.");

    if (vehData.reloadDepots.empty() && trips_.size() > 1)
        throw std::invalid_argument("Vehicle cannot perform multiple trips.");

    if (trips_[0].startDepot() != startDepot_)
    {
        auto const *msg = "Route must start at vehicle's start_depot.";
        throw std::invalid_argument(msg);
    }

    if (trips_.back().endDepot() != endDepot_)
        throw std::invalid_argument("Route must end at vehicle's end_depot.");

    for (auto const &trip : trips_)
        if (trip.vehicleType() != vehicleType_)
        {
            auto const *msg = "Each trip must use the route's vehicle type.";
            throw std::invalid_argument(msg);
        }

    for (size_t idx = 0; idx + 1 != trips_.size(); ++idx)
        if (trips_[idx].endDepot() != trips_[idx + 1].startDepot())
        {
            auto *msg = "Consecutive trips must start at previous' end_depot.";
            throw std::invalid_argument(msg);
        }
}

void Route::makeSchedule(ProblemData const &data)
{
    // TODO release time / multi trip
    schedule_.clear();
    schedule_.reserve(size() + 2 * numTrips());  // clients and start/end depots

    auto const &vehData = data.vehicleType(vehicleType_);
    auto const &durations = data.durationMatrix(vehData.profile);

    auto now = startTime_;
    for (size_t tripIdx = 0; tripIdx != trips_.size(); ++tripIdx)
    {
        auto const handle
            = [&](auto const &where, size_t location, Duration service)
        {
            auto const wait = std::max<Duration>(where.twEarly - now, 0);
            auto const tw = std::max<Duration>(now - where.twLate, 0);

            now += wait;
            now -= tw;

            schedule_.emplace_back(
                location, tripIdx, now, now + service, wait, tw);

            now += service;
        };

        auto const &trip = trips_[tripIdx];

        ProblemData::Depot const &start = data.location(trip.startDepot());
        handle(start, trip.startDepot(), start.serviceDuration);

        size_t prevClient = trip.startDepot();
        for (auto const client : trip)
        {
            now += durations(prevClient, client);

            ProblemData::Client const &clientData = data.location(client);
            handle(clientData, client, clientData.serviceDuration);

            prevClient = client;
        }

        now += durations(prevClient, trip.endDepot());

        ProblemData::Depot const &end = data.location(trip.endDepot());
        handle(end, trip.endDepot(), 0);
    }
}

Route::Route(ProblemData const &data, Visits visits, size_t vehicleType)
    : Route(data, {{data, std::move(visits), vehicleType}}, vehicleType)
{
}

Route::Route(ProblemData const &data, Trips trips, size_t vehType)
    : trips_(std::move(trips)),
      delivery_(data.numLoadDimensions(), 0),
      pickup_(data.numLoadDimensions(), 0),
      excessLoad_(data.numLoadDimensions(), 0),
      vehicleType_(vehType)
{
    if (trips_.empty())  // then we insert a dummy trip for ease.
        trips_.emplace_back(data, Visits{}, vehType);

    auto const &vehData = data.vehicleType(vehType);
    startDepot_ = vehData.startDepot;
    endDepot_ = vehData.endDepot;

    validate(data);

    for (auto const &trip : trips_)  // general statistics
    {
        distance_ += trip.distance();
        service_ += trip.serviceDuration();
        travel_ += trip.travelDuration();
        prizes_ += trip.prizes();

        auto const [x, y] = trip.centroid();
        centroid_.first += (x * trip.size()) / size();
        centroid_.second += (y * trip.size()) / size();
    }

    distanceCost_ = vehData.unitDistanceCost * static_cast<Cost>(distance_);
    excessDistance_ = std::max<Distance>(distance_ - vehData.maxDistance, 0);

    for (size_t idx = 0; idx != trips_.size(); ++idx)  // load statistics
    {
        auto const &trip = trips_[idx];
        auto const &tripDeliv = trip.delivery();
        auto const &tripPick = trip.pickup();
        auto const &tripLoad = trip.load();

        for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
        {
            LoadSegment ls = {tripDeliv[dim], tripPick[dim], tripLoad[dim], 0};

            if (idx == 0 && vehData.initialLoad[dim] > 0)
                // This is initial load that the first trip does not know about
                // that we need to account for first.
                ls = LoadSegment::merge({vehData, dim}, ls);

            delivery_[dim] += ls.delivery();
            pickup_[dim] += ls.pickup();
            excessLoad_[dim] += ls.excessLoad(vehData.capacity[dim]);
        }
    }

    // Duration statistics.
    auto const &durations = data.durationMatrix(vehData.profile);
    Duration startTime = 0;
    for (size_t idx = 0; idx != numTrips(); ++idx)
    {
        auto const &trip = trips_[idx];

        ProblemData::Depot const &start = data.location(trip.startDepot());
        DurationSegment ds = {start, start.serviceDuration};
        if (idx == 0)  // vehicle to first depot
            ds = DurationSegment::merge(0, {vehData, vehData.startLate}, ds);

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

        if (idx == numTrips() - 1)  // last depot to vehicle
            ds = DurationSegment::merge(0, ds, {vehData, vehData.twLate});

        duration_ += ds.duration();
        timeWarp_ += ds.timeWarp();

        if (idx == 0)
        {
            startTime = ds.twEarly();
            startTime_ = ds.twEarly();
        }

        // Slack is always the minimum across trips; in particular, if there is
        // a trip with time warp, then there is no slack.
        slack_ = std::min(slack_, ds.twLate() - ds.twEarly());

        if (ds.twLate() < startTime)  // time warp: we need to start before
        {                             // we feasibly can.
            slack_ = 0;
            timeWarp_ += startTime - ds.twLate();
            startTime = ds.twLate() + ds.duration() - ds.timeWarp();
        }
        else  // then we can start at a feasible time, but there might be a
        {     // bit of extra waiting time.
            auto const start = std::max(startTime, ds.twEarly());
            duration_ += start - startTime;
            startTime = start + ds.duration() - ds.timeWarp();
        }
    }

    durationCost_ = vehData.unitDurationCost * static_cast<Cost>(duration_);
    timeWarp_ += duration_ - timeWarp_ > vehData.maxDuration
                     ? duration_ - timeWarp_ - vehData.maxDuration
                     : 0;

    makeSchedule(data);
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

size_t Route::numTrips() const { return trips_.size(); }

Client Route::operator[](size_t idx) const
{
    for (auto const &trip : trips_)
        if (idx < trip.size())
            return trip[idx];
        else
            idx -= trip.size();

    throw std::out_of_range("Index out of range.");
}

Route::Iterator Route::begin() const { return Iterator(*this, 0); }

Route::Iterator Route::end() const { return Iterator(*this, size()); }

Route::Trips const &Route::trips() const { return trips_; }

Trip const &Route::trip(size_t idx) const
{
    assert(idx < trips_.size());
    return trips_[idx];
}

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

Duration Route::releaseTime() const { return trips_[0].releaseTime(); }

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
        if (idx != 0)
            out << " | ";
        out << trips[idx];
    }

    return out;
}
