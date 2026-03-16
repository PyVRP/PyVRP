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

Route::ScheduledActivity::ScheduledActivity(Activity activity,
                                            size_t trip,
                                            Duration startTime,
                                            Duration endTime,
                                            Duration waitDuration,
                                            Duration timeWarp)
    : Activity(activity),
      trip_(trip),
      startTime_(startTime),
      endTime_(endTime),
      waitDuration_(waitDuration),
      timeWarp_(timeWarp)
{
    assert(startTime_ <= endTime_);
}

size_t Route::ScheduledActivity::trip() const { return trip_; }

Duration Route::ScheduledActivity::startTime() const { return startTime_; }

Duration Route::ScheduledActivity::endTime() const { return endTime_; }

Duration Route::ScheduledActivity::duration() const
{
    return endTime_ - startTime_;
}

Duration Route::ScheduledActivity::waitDuration() const
{
    return waitDuration_;
}

Duration Route::ScheduledActivity::timeWarp() const { return timeWarp_; }

void Route::validate(ProblemData const &data,
                     Activities const &activities) const
{
    auto const &vehData = data.vehicleType(vehicleType_);

    size_t numTrips = 1;
    for (auto const &activity : activities)  // some quick checks up front
    {
        if (activity.isDepot())
        {
            numTrips++;  // this is a reload depot, so we start another trip

            if (activity.idx() >= data.numDepots())
            {
                std::ostringstream msg;
                msg << "Depot " << activity << " is not understood.";
                throw std::invalid_argument(msg.str());
            }
        }

        if (activity.isClient() && activity.idx() >= data.numClients())
        {
            std::ostringstream msg;
            msg << "Client " << activity << " is not understood.";
            throw std::invalid_argument(msg.str());
        }
    }

    if (numTrips > vehData.maxTrips())
        throw std::invalid_argument("Vehicle cannot perform this many trips.");
}

void Route::setSchedule(ProblemData const &data, Activities const &activities)
{
    schedule_.reserve(activities.size() + 2);  // incl. start and end depots

    auto const &vehData = data.vehicleType(vehicleType_);
    auto const &durations = data.durationMatrix(vehData.profile);

    std::vector<Duration> releaseTimes;  // per trip, for the schedule

    auto const &end = data.depot(vehData.endDepot);
    auto ds = DurationSegment::merge({end, 0}, {vehData, vehData.twLate});
    size_t nextLoc = end.location;
    for (auto it = activities.rbegin(); it != activities.rend(); ++it)
    {
        if (it->isDepot())
        {
            auto const &depot = data.depot(it->idx());

            auto const depotService = depot.serviceDuration;
            service_ += depotService;

            auto const edgeDur = durations(depot.location, nextLoc);
            travel_ += edgeDur;

            ds = DurationSegment::merge(edgeDur, {depot, depotService}, ds);
            releaseTimes.insert(releaseTimes.begin(), ds.releaseTime());

            ds = ds.finaliseFront();
            nextLoc = depot.location;
        }
        else
        {
            auto const &clientData = data.client(it->idx());
            service_ += clientData.serviceDuration;

            auto const edgeDur = durations(clientData.location, nextLoc);
            travel_ += edgeDur;

            ds = DurationSegment::merge(edgeDur, {clientData}, ds);
            nextLoc = clientData.location;
        }
    }

    auto const &start = data.depot(vehData.startDepot);
    auto const edgeDur = durations(start.location, nextLoc);

    service_ += start.serviceDuration;
    travel_ += edgeDur;

    ds = DurationSegment::merge(edgeDur, {start, start.serviceDuration}, ds);
    ds = DurationSegment::merge({vehData, vehData.startLate}, ds);

    releaseTimes.insert(releaseTimes.begin(), ds.releaseTime());

    duration_ = ds.duration();
    overtime_ = std::max<Duration>(duration_ - vehData.shiftDuration, 0);
    durationCost_ = vehData.unitDurationCost * static_cast<Cost>(duration_)
                    + vehData.unitOvertimeCost * static_cast<Cost>(overtime_);
    startTime_ = ds.startEarly();
    releaseTime_ = ds.releaseTime();
    slack_ = ds.slack();
    timeWarp_ = ds.timeWarp(vehData.maxDuration);

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

    handle({Activity::ActivityType::DEPOT, vehData.startDepot},
           0,
           std::max(start.twEarly, std::min(releaseTime_, start.twLate)),
           std::min(start.twLate, vehData.startLate),
           start.serviceDuration);

    size_t prevLoc = start.location;
    for (size_t tripIdx = 0; auto const &activity : activities)
        if (activity.isDepot())
        {
            auto const releaseTime = releaseTimes[++tripIdx];

            auto const &depot = data.depot(activity.idx());
            now += durations(prevLoc, depot.location);

            handle(activity,
                   tripIdx,
                   std::max(depot.twEarly, std::min(releaseTime, depot.twLate)),
                   depot.twLate,
                   depot.serviceDuration);

            prevLoc = depot.location;
        }
        else
        {
            auto const &clientData = data.client(activity.idx());
            now += durations(prevLoc, clientData.location);

            handle(activity,
                   tripIdx,
                   clientData.twEarly,
                   clientData.twLate,
                   clientData.serviceDuration);

            prevLoc = clientData.location;
        }

    now += durations(prevLoc, end.location);
    handle({Activity::ActivityType::DEPOT, vehData.endDepot},
           releaseTimes.size(),
           end.twEarly,
           end.twLate,
           0);
}

void Route::setDistance(ProblemData const &data)
{
    auto const &vehData = data.vehicleType(vehicleType_);
    auto const &distances = data.distanceMatrix(vehData.profile);

    size_t frmLoc = data.depot(startDepot()).location;
    for (size_t idx = 1; idx != schedule_.size(); ++idx)
    {
        auto const &activity = schedule_[idx];
        auto const toLoc = activity.isDepot()
                               ? data.depot(activity.idx()).location
                               : data.client(activity.idx()).location;

        distance_ += distances(frmLoc, toLoc);
        frmLoc = toLoc;
    }

    distanceCost_ = vehData.unitDistanceCost * static_cast<Cost>(distance_);
    excessDistance_ = std::max<Distance>(distance_ - vehData.maxDistance, 0);
}

void Route::setLoad(ProblemData const &data)
{
    auto const &vehData = data.vehicleType(vehicleType_);

    for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
    {
        LoadSegment ls;

        if (vehData.initialLoad[dim] > 0)  // start with initial vehicle load
            ls = {vehData, dim};

        for (size_t idx = 1; idx != schedule_.size(); ++idx)
        {
            auto const &activity = schedule_[idx];

            if (activity.isDepot())
            {
                delivery_[dim] += ls.delivery();
                pickup_[dim] += ls.pickup();
                ls = ls.finalise(vehData.capacity[dim]);
            }

            if (activity.isClient())
                ls = LoadSegment::merge(ls, {data.client(activity.idx()), dim});
        }

        excessLoad_[dim] = ls.excessLoad(vehData.capacity[dim]);
    }
}

void Route::setOtherStatistics(ProblemData const &data)
{
    auto const &vehData = data.vehicleType(vehicleType_);
    fixedVehicleCost_ = vehData.fixedCost;

    for (auto const &activity : schedule_)
        if (activity.isClient())
            prizes_ += data.client(activity.idx()).prize;
}

Route::Route(ProblemData const &data,
             std::vector<Client> const &visits,
             VehicleType vehicleType)
{
    std::vector<Activity> activities;
    activities.reserve(visits.size());

    for (auto const client : visits)
        activities.emplace_back(Activity::ActivityType::CLIENT, client);

    *this = Route(data, activities, vehicleType);
}

Route::Route(ProblemData const &data,
             Activities const &activities,
             size_t vehType)
    : delivery_(data.numLoadDimensions(), 0),
      pickup_(data.numLoadDimensions(), 0),
      excessLoad_(data.numLoadDimensions(), 0),
      vehicleType_(vehType)
{
    validate(data, activities);
    setSchedule(data, activities);  // duration statistics and route schedule
    setDistance(data);              // distance statistics
    setLoad(data);                  // load statistics
    setOtherStatistics(data);       // e.g. prizes, fixed cost
}

Route::Route(Schedule schedule,
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
             size_t vehicleType)
    : schedule_(std::move(schedule)),
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
      vehicleType_(vehicleType)
{
}

bool Route::empty() const
{
    return size() == 2;  // only start and end depot activities
}

size_t Route::size() const { return schedule_.size(); }

size_t Route::numClients() const { return size() - numDepots(); }

size_t Route::numDepots() const { return numTrips() + 1; }

size_t Route::numTrips() const { return schedule_.back().trip(); }

Route::ScheduledActivity const &Route::operator[](size_t idx) const
{
    if (idx >= size())
        throw std::out_of_range("Index out of range.");

    return schedule_[idx];
}

Route::Schedule::const_iterator Route::begin() const
{
    return schedule_.begin();
}

Route::Schedule::const_iterator Route::end() const { return schedule_.end(); }

Route::Schedule const &Route::schedule() const { return schedule_; }

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

size_t Route::startDepot() const
{
    auto const &activity = schedule_.front();

    assert(activity.isDepot());
    return activity.idx();
}

size_t Route::endDepot() const
{
    auto const &activity = schedule_.back();

    assert(activity.isDepot());
    return activity.idx();
}

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
        && schedule_ == other.schedule_;
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

std::ostream &operator<<(std::ostream &out, Route const &route)
{
    for (size_t idx = 1; idx != route.size() - 1; ++idx)
    {
        auto const &activity = route[idx];
        if (activity.isDepot())
            out << '|';
        else
            out << activity;

        if (idx < route.size() - 2)  // then we'll insert more after this
            out << ' ';
    }

    return out;
}
