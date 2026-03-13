#include "Route.h"
#include "DurationSegment.h"
#include "LoadSegment.h"

#include <algorithm>
#include <cassert>
#include <fstream>

using pyvrp::Activity;
using pyvrp::Cost;
using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Load;
using pyvrp::Route;

using Client = size_t;

Route::ScheduledVisit::ScheduledVisit(Activity activity,
                                      size_t trip,
                                      Duration startService,
                                      Duration endService,
                                      Duration waitDuration,
                                      Duration timeWarp)
    : activity_(activity),
      trip_(trip),
      startService_(startService),
      endService_(endService),
      waitDuration_(waitDuration),
      timeWarp_(timeWarp)
{
    assert(startService_ <= endService_);
}

Activity Route::ScheduledVisit::activity() const { return activity_; }

size_t Route::ScheduledVisit::trip() const { return trip_; }

Duration Route::ScheduledVisit::startService() const { return startService_; }

Duration Route::ScheduledVisit::endService() const { return endService_; }

Duration Route::ScheduledVisit::serviceDuration() const
{
    return endService_ - startService_;
}

Duration Route::ScheduledVisit::waitDuration() const { return waitDuration_; }

Duration Route::ScheduledVisit::timeWarp() const { return timeWarp_; }

void Route::makeSchedule(ProblemData const &data)
{
    schedule_.clear();
    schedule_.reserve(activities_.size());

    auto const &vehData = data.vehicleType(vehicleType_);
    auto const &durations = data.durationMatrix(vehData.profile);

    Duration releaseTime;
    std::vector<Duration> releaseTimes;  // per trip
    for (auto it = activities_.begin() + 1; it != activities_.end(); ++it)
    {
        if (it->isDepot())
        {
            releaseTimes.push_back(releaseTime);
            releaseTime = 0;
            continue;
        }

        auto const &client = data.client(it->idx);
        releaseTime = std::max(releaseTime, client.releaseTime);
    }
    releaseTimes.push_back(0);

    auto now = startTime_;
    auto const handle = [&](Activity activity,
                            size_t trip,
                            Duration early,
                            Duration late,
                            Duration service)
    {
        auto const wait = std::max<Duration>(early - now, 0);
        auto const tw = std::max<Duration>(now - late, 0);

        now += wait;
        now -= tw;

        schedule_.emplace_back(activity, trip, now, now + service, wait, tw);

        now += service;
    };

    size_t tripIdx = 0;
    size_t prevLoc = data.depot(startDepot_).location;
    for (size_t idx = 0; idx != activities_.size(); ++idx)
    {
        auto const activity = activities_[idx];

        if (activity.isDepot())
        {
            auto const &depot = data.depot(activity.idx);
            now += durations(prevLoc, depot.location);

            Duration earliestStart = depot.twEarly;
            Duration latestStart = depot.twLate;
            Duration serviceDuration = depot.serviceDuration;

            if (idx == activities_.size() - 1)
                serviceDuration = 0;
            else
                earliestStart
                    = std::max(depot.twEarly,
                               std::min(releaseTimes[tripIdx++], depot.twLate));

            if (idx == 0)  // first trip accounts for latest start
                latestStart = std::min(depot.twLate, vehData.startLate);

            handle(
                activity, tripIdx, earliestStart, latestStart, serviceDuration);

            prevLoc = depot.location;
        }
        else
        {
            auto const &clientData = data.client(activity.idx);
            now += durations(prevLoc, clientData.location);

            handle(activity,
                   tripIdx,
                   clientData.twEarly,
                   clientData.twLate,
                   clientData.serviceDuration);

            prevLoc = clientData.location;
        }
    }
}

Route::Route(ProblemData const &data,
             std::vector<Client> visits,
             VehicleType vehicleType)
{
    std::vector<Activity> activities;
    activities.reserve(visits.size());

    for (auto const client : visits)
        activities.emplace_back(Activity::ActivityType::CLIENT, client);

    *this = Route(data, activities, vehicleType);
}

Route::Route(ProblemData const &data, Activities activities, size_t vehType)
    : activities_(std::move(activities)),
      delivery_(data.numLoadDimensions(), 0),
      pickup_(data.numLoadDimensions(), 0),
      excessLoad_(data.numLoadDimensions(), 0),
      vehicleType_(vehType)
{
    auto const &vehData = data.vehicleType(vehicleType_);
    startDepot_ = vehData.startDepot;
    endDepot_ = vehData.endDepot;
    fixedVehicleCost_ = vehData.fixedCost;

    activities_.insert(activities_.begin(),
                       {Activity::ActivityType::DEPOT, startDepot_});
    activities_.emplace_back(Activity::ActivityType::DEPOT, endDepot_);

    for (auto const &activity : activities_)
    {
        if (activity.isDepot() && activity.idx >= data.numDepots())
        {
            std::ostringstream msg;
            msg << "Depot " << activity << " is not understood.";
            throw std::invalid_argument(msg.str());
        }

        if (activity.isClient() && activity.idx >= data.numClients())
        {
            std::ostringstream msg;
            msg << "Client " << activity << " is not understood.";
            throw std::invalid_argument(msg.str());
        }
    }

    // Distance.
    auto const &distances = data.distanceMatrix(vehData.profile);
    size_t frmLoc = data.depot(startDepot_).location;
    for (size_t idx = 1; idx != activities_.size(); ++idx)
    {
        auto const toLoc = activities_[idx].isDepot()
                               ? data.depot(activities_[idx].idx).location
                               : data.client(activities_[idx].idx).location;

        distance_ += distances(frmLoc, toLoc);
        frmLoc = toLoc;
    }

    distanceCost_ = vehData.unitDistanceCost * static_cast<Cost>(distance_);
    excessDistance_ = std::max<Distance>(distance_ - vehData.maxDistance, 0);

    // Prizes.
    for (auto const &activity : activities_)
        if (activity.isClient())
            prizes_ += data.client(activity.idx).prize;

    // Load.
    for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
    {
        LoadSegment ls;

        if (vehData.initialLoad[dim] > 0)  // start with initial vehicle load
            ls = {vehData, dim};

        for (size_t idx = 1; idx != activities_.size(); ++idx)
        {
            if (activities_[idx].isDepot())
            {
                delivery_[dim] += ls.delivery();
                pickup_[dim] += ls.pickup();
                ls = ls.finalise(vehData.capacity[dim]);
            }

            if (activities_[idx].isClient())
                ls = LoadSegment::merge(
                    ls, {data.client(activities_[idx].idx), dim});
        }

        excessLoad_[dim] = ls.excessLoad(vehData.capacity[dim]);
    }

    // Duration. We iterate in reverse, that is, from the last to
    // the first visit.
    auto const &durations = data.durationMatrix(vehData.profile);
    DurationSegment ds = {vehData, vehData.twLate};
    size_t nextLoc = data.depot(endDepot_).location;
    for (auto idx = activities_.size(); idx-- > 0;)
    {
        auto const activity = activities_[idx];
        if (activity.isDepot())
        {
            auto const &depot = data.depot(activity.idx);
            auto const edgeDuration = durations(depot.location, nextLoc);
            travel_ += edgeDuration;

            auto const depotService
                = idx == activities_.size() - 1 ? 0 : depot.serviceDuration;

            service_ += depotService;
            ds = DurationSegment::merge(
                edgeDuration, {depot, depotService}, ds);

            if (idx != 0 && idx != activities_.size() - 1)  // is reload depot
                ds = ds.finaliseFront();

            nextLoc = depot.location;
        }
        else
        {
            auto const &clientData = data.client(activity.idx);
            service_ += clientData.serviceDuration;

            auto const edgeDuration = durations(clientData.location, nextLoc);
            travel_ += edgeDuration;

            ds = DurationSegment::merge(edgeDuration, {clientData}, ds);
            nextLoc = clientData.location;
        }
    }

    ds = DurationSegment::merge({vehData, vehData.startLate}, ds);

    duration_ = ds.duration();
    overtime_ = std::max<Duration>(duration_ - vehData.shiftDuration, 0);
    durationCost_ = vehData.unitDurationCost * static_cast<Cost>(duration_)
                    + vehData.unitOvertimeCost * static_cast<Cost>(overtime_);
    startTime_ = ds.startEarly();
    releaseTime_ = ds.releaseTime();
    slack_ = ds.slack();
    timeWarp_ = ds.timeWarp(vehData.maxDuration);

    makeSchedule(data);

    if (numTrips() > vehData.maxTrips())
        throw std::invalid_argument("Vehicle cannot perform this many trips.");
}

Route::Route(std::vector<Activity> activities,
             Distance distance,
             Cost distanceCost,
             Distance excessDistance,
             std::vector<Load> delivery,
             std::vector<Load> pickup,
             std::vector<Load> excessLoad,
             Duration duration,
             Duration overtime,
             Cost durationCost,
             Duration timeWarp,
             Duration travel,
             Duration service,
             Duration startTime,
             Duration releaseTime,
             Duration slack,
             Cost prizes,
             size_t vehicleType,
             size_t startDepot,
             size_t endDepot,
             std::vector<ScheduledVisit> schedule)
    : activities_(std::move(activities)),
      schedule_(std::move(schedule)),
      distance_(distance),
      distanceCost_(distanceCost),
      excessDistance_(excessDistance),
      delivery_(std::move(delivery)),
      pickup_(std::move(pickup)),
      excessLoad_(std::move(excessLoad)),
      duration_(duration),
      overtime_(overtime),
      durationCost_(durationCost),
      timeWarp_(timeWarp),
      travel_(travel),
      service_(service),
      startTime_(startTime),
      releaseTime_(releaseTime),
      slack_(slack),
      prizes_(prizes),
      vehicleType_(vehicleType),
      startDepot_(startDepot),
      endDepot_(endDepot)
{
}

bool Route::empty() const
{
    return size() == 2;  // only start and end depot activities
}

size_t Route::size() const { return activities_.size(); }

size_t Route::numClients() const
{
    // There are #trips + 1 depots in the route, so size() - #trips - 1 is the
    // number of clients.
    return size() - numTrips() - 1;
}

size_t Route::numTrips() const { return schedule_.back().trip(); }

Activity Route::operator[](size_t idx) const
{
    if (idx >= size())
        throw std::out_of_range("Index out of range.");

    return activities_[idx];
}

Route::Activities::const_iterator Route::begin() const
{
    return activities_.begin();
}

Route::Activities::const_iterator Route::end() const
{
    return activities_.end();
}

Route::Activities const &Route::activities() const { return activities_; }

std::vector<Route::ScheduledVisit> const &Route::schedule() const
{
    return schedule_;
}

Cost Route::fixedVehicleCost() const { return fixedVehicleCost_; }

Distance Route::distance() const { return distance_; }

Cost Route::distanceCost() const { return distanceCost_; }

Distance Route::excessDistance() const { return excessDistance_; }

std::vector<Load> const &Route::delivery() const { return delivery_; }

std::vector<Load> const &Route::pickup() const { return pickup_; }

std::vector<Load> const &Route::excessLoad() const { return excessLoad_; }

Duration Route::duration() const { return duration_; }

Duration Route::overtime() const { return overtime_; }

Cost Route::durationCost() const { return durationCost_; }

Duration Route::serviceDuration() const { return service_; }

Duration Route::timeWarp() const { return timeWarp_; }

Duration Route::waitDuration() const { return duration_ - travel_ - service_; }

Duration Route::travelDuration() const { return travel_; }

Duration Route::startTime() const { return startTime_; }

Duration Route::endTime() const { return startTime_ + duration_ - timeWarp_; }

Duration Route::slack() const { return slack_; }

Duration Route::releaseTime() const { return releaseTime_; }

Cost Route::prizes() const { return prizes_; }

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
    // Only when these are the same we test if the activities are all equal.
    // clang-format off
    return distance_ == other.distance_
        && duration_ == other.duration_
        && timeWarp_ == other.timeWarp_
        && vehicleType_ == other.vehicleType_
        && activities_ == other.activities_;
    // clang-format on
}

template <> Cost pyvrp::CostEvaluator::penalisedCost(Route const &route) const
{
    if (route.empty())
        return 0;

    // clang-format off
    return route.distanceCost()
         + route.durationCost()
         + route.fixedVehicleCost()
         + excessLoadPenalties(route.excessLoad())
         + twPenalty(route.timeWarp())
         + distPenalty(route.excessDistance(), 0);
    // clang-format on
}

std::ostream &operator<<(std::ostream &out, Route::ScheduledVisit const &visit)
{
    return out << visit.activity();
}

std::ostream &operator<<(std::ostream &out, Route const &route)
{
    auto const &activities = route.activities();
    for (size_t idx = 1; idx != activities.size() - 1; ++idx)
    {
        auto const &activity = activities[idx];
        if (activity.isDepot())
            out << '|';
        else
            out << activity;

        if (idx < activities.size() - 2)  // then we'll insert more after this
            out << ' ';
    }

    return out;
}
