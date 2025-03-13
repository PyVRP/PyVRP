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

using Client = size_t;

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

Route::Route(ProblemData const &data, Visits visits, size_t const vehicleType)
    : visits_(std::move(visits)), centroid_({0, 0}), vehicleType_(vehicleType)
{
    auto const &vehType = data.vehicleType(vehicleType);
    startDepot_ = vehType.startDepot;
    endDepot_ = vehType.endDepot;

    ProblemData::Depot const &start = data.location(startDepot_);
    service_ += start.serviceDuration;

    DurationSegment const vehStart(vehType, vehType.startLate);
    DurationSegment const depotStart(start, start.serviceDuration);
    DurationSegment ds = DurationSegment::merge(0, vehStart, depotStart);

    std::vector<LoadSegment> loadSegments(data.numLoadDimensions());
    for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
        loadSegments[dim] = {vehType, dim};

    auto const &distances = data.distanceMatrix(vehType.profile);
    auto const &durations = data.durationMatrix(vehType.profile);

    for (size_t prevClient = startDepot_; auto const client : visits_)
    {
        ProblemData::Client const &clientData = data.location(client);

        distance_ += distances(prevClient, client);
        service_ += clientData.serviceDuration;
        prizes_ += clientData.prize;

        auto const edgeDuration = durations(prevClient, client);
        travel_ += edgeDuration;
        ds = DurationSegment::merge(edgeDuration, ds, {clientData});

        centroid_.first += static_cast<double>(clientData.x) / size();
        centroid_.second += static_cast<double>(clientData.y) / size();

        for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
        {
            auto const clientLs = LoadSegment(clientData, dim);
            loadSegments[dim] = LoadSegment::merge(loadSegments[dim], clientLs);
        }

        prevClient = client;
    }

    auto const last = visits_.empty() ? startDepot_ : visits_.back();
    distance_ += distances(last, endDepot_);
    distanceCost_ = vehType.unitDistanceCost * static_cast<Cost>(distance_);
    excessDistance_ = std::max<Distance>(distance_ - vehType.maxDistance, 0);

    travel_ += durations(last, endDepot_);

    delivery_.reserve(data.numLoadDimensions());
    pickup_.reserve(data.numLoadDimensions());
    excessLoad_.reserve(data.numLoadDimensions());
    for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
    {
        delivery_.push_back(loadSegments[dim].delivery());
        pickup_.push_back(loadSegments[dim].pickup());
        excessLoad_.push_back(std::max<Load>(
            loadSegments[dim].load() - vehType.capacity[dim], 0));
    }

    DurationSegment const depotEnd(vehType, vehType.twLate);
    DurationSegment const vehEnd(data.location(endDepot_), 0);
    DurationSegment const endDS = DurationSegment::merge(0, depotEnd, vehEnd);
    ds = DurationSegment::merge(durations(last, endDepot_), ds, endDS);

    duration_ = ds.duration();
    durationCost_ = vehType.unitDurationCost * static_cast<Cost>(duration_);
    startTime_ = ds.twEarly();
    slack_ = ds.twLate() - ds.twEarly();
    timeWarp_ = ds.timeWarp(vehType.maxDuration);
    release_ = ds.releaseTime();

    schedule_.reserve(size());
    auto now = startTime_;
    for (size_t prevClient = startDepot_; auto const client : visits_)
    {
        now += durations(prevClient, client);

        ProblemData::Client const &clientData = data.location(client);
        auto const wait = std::max<Duration>(clientData.twEarly - now, 0);
        auto const timeWarp = std::max<Duration>(now - clientData.twLate, 0);

        now += wait;
        now -= timeWarp;

        schedule_.emplace_back(
            now, now + clientData.serviceDuration, wait, timeWarp);

        now += clientData.serviceDuration;
        prevClient = client;
    }
}

Route::Route(Visits visits,
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
             size_t endDepot,
             std::vector<ScheduledVisit> schedule)
    : visits_(std::move(visits)),
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

bool Route::empty() const { return visits_.empty(); }

size_t Route::size() const { return visits_.size(); }

Client Route::operator[](size_t idx) const { return visits_[idx]; }

Route::Visits::const_iterator Route::begin() const { return visits_.cbegin(); }

Route::Visits::const_iterator Route::end() const { return visits_.cend(); }

Route::Visits const &Route::visits() const { return visits_; }

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
        && visits_ == other.visits_;
    // clang-format on
}

std::ostream &operator<<(std::ostream &out, Route const &route)
{
    for (auto const client : route)
        out << client << ' ';
    return out;
}
