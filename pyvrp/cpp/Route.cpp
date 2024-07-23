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

Route::Route(ProblemData const &data, Visits visits, size_t const vehicleType)
    : visits_(std::move(visits)), centroid_({0, 0}), vehicleType_(vehicleType)
{
    auto const &vehType = data.vehicleType(vehicleType);
    startLocation_ = vehType.startLocation;
    endLocation_ = vehType.endLocation;

    DurationSegment ds = {startLocation_, vehType};
    auto ls = LoadSegment(0, 0, 0);
    size_t prevLocation = startLocation_;

    auto const &distances = data.distanceMatrix(vehType.profile);
    auto const &durations = data.durationMatrix(vehType.profile);

    for (size_t const visit : visits_)
    {
        auto const &client = data.client(visit);
        service_ += client.serviceDuration;
        prizes_ += client.prize;

        distance_ += distances(prevLocation, client.location);
        travel_ += durations(prevLocation, client.location);

        auto const &location = data.location(client.location);
        centroid_.first += static_cast<double>(location.x) / size();
        centroid_.second += static_cast<double>(location.y) / size();

        ds = DurationSegment::merge(durations, ds, DurationSegment(client));
        ls = LoadSegment::merge(ls, LoadSegment(client));

        prevLocation = client.location;
    }

    distance_ += distances(prevLocation, endLocation_);
    distanceCost_ = vehType.unitDistanceCost * static_cast<Cost>(distance_);
    excessDistance_ = std::max<Distance>(distance_ - vehType.maxDistance, 0);

    travel_ += durations(prevLocation, endLocation_);

    delivery_ = ls.delivery();
    pickup_ = ls.pickup();
    excessLoad_ = std::max<Load>(ls.load() - vehType.capacity, 0);

    DurationSegment endDS(endLocation_, vehType);
    ds = DurationSegment::merge(durations, ds, endDS);
    duration_ = ds.duration();
    durationCost_ = vehType.unitDurationCost * static_cast<Cost>(duration_);
    startTime_ = ds.twEarly();
    slack_ = ds.twLate() - ds.twEarly();
    timeWarp_ = ds.timeWarp(vehType.maxDuration);
    release_ = ds.releaseTime();
}

Route::Route(Visits visits,
             Distance distance,
             Cost distanceCost,
             Distance excessDistance,
             Load delivery,
             Load pickup,
             Load excessLoad,
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
             size_t startLocation,
             size_t endLocation)
    : visits_(std::move(visits)),
      distance_(distance),
      distanceCost_(distanceCost),
      excessDistance_(excessDistance),
      delivery_(delivery),
      pickup_(pickup),
      excessLoad_(excessLoad),
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
      startLocation_(startLocation),
      endLocation_(endLocation)
{
}

bool Route::empty() const { return visits_.empty(); }

size_t Route::size() const { return visits_.size(); }

size_t Route::operator[](size_t idx) const { return visits_[idx]; }

Route::Visits::const_iterator Route::begin() const { return visits_.cbegin(); }

Route::Visits::const_iterator Route::end() const { return visits_.cend(); }

Route::Visits const &Route::visits() const { return visits_; }

Distance Route::distance() const { return distance_; }

Cost Route::distanceCost() const { return distanceCost_; }

Distance Route::excessDistance() const { return excessDistance_; }

Load Route::delivery() const { return delivery_; }

Load Route::pickup() const { return pickup_; }

Load Route::excessLoad() const { return excessLoad_; }

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

size_t Route::startLocation() const { return startLocation_; }

size_t Route::endLocation() const { return endLocation_; }

bool Route::isFeasible() const
{
    return !hasExcessLoad() && !hasTimeWarp() && !hasExcessDistance();
}

bool Route::hasExcessLoad() const { return excessLoad_ > 0; }

bool Route::hasExcessDistance() const { return excessDistance_ > 0; }

bool Route::hasTimeWarp() const { return timeWarp_ > 0; }

bool Route::operator==(Route const &other) const
{
    // First compare simple attributes, since that's a quick and cheap check.
    // Only when these are the same we test if the visits are all equal.
    // clang-format off
    return distance_ == other.distance_
        && delivery_ == other.delivery_
        && pickup_ == other.pickup_
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
