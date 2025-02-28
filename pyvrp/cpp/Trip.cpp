#include "Trip.h"
#include "DurationSegment.h"
#include "LoadSegment.h"

using pyvrp::Trip;

Trip::Trip(ProblemData const &data,
           Visits visits,
           size_t const vehicleType,
           size_t const startDepot,
           size_t const endDepot,
           Trip const *after)
    : visits_(std::move(visits)),
      vehicleType_(vehicleType),
      startDepot_(startDepot),
      endDepot_(endDepot)
{
    if (startDepot_ >= data.numDepots())
        throw std::invalid_argument("Start depot not understood.");

    if (endDepot_ >= data.numDepots())
        throw std::invalid_argument("End depot not understood");

    auto const &vehType = data.vehicleType(vehicleType_);

    ProblemData::Depot const &start = data.location(startDepot_);
    service_ += start.serviceDuration;

    DurationSegment const vehStart(vehType, vehType.startLate);
    DurationSegment const depotStart(start, start.serviceDuration);
    DurationSegment ds = DurationSegment::merge(0, vehStart, depotStart);

    if (after)
    {
        if (after->endDepot_ != startDepot_)
            throw std::invalid_argument("Trip start unequal to previous end.");

        // Use attributes of previous trip to determine previous duration
        // segment, ignoring release times since those only apply per-trip.
        DurationSegment prev = {after->duration_,
                                after->timeWarp_,
                                after->startTime_,
                                after->startTime_ + after->slack_,
                                0};

        ds = DurationSegment::merge(0, prev, ds);
    }

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

    auto const last = empty() ? startDepot_ : visits_.back();
    distance_ += distances(last, endDepot_);
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
    startTime_ = ds.twEarly();
    slack_ = ds.twLate() - ds.twEarly();
    timeWarp_ = ds.timeWarp(vehType.maxDuration);
    release_ = ds.releaseTime();
}

bool Trip::empty() const { return visits_.empty(); }

size_t Trip::size() const { return visits_.size(); }

Trip::Visits const &Trip::visits() const { return visits_; }

pyvrp::Distance Trip::distance() const { return distance_; }

std::vector<pyvrp::Load> const &Trip::delivery() const { return delivery_; }

std::vector<pyvrp::Load> const &Trip::pickup() const { return pickup_; }

std::vector<pyvrp::Load> const &Trip::excessLoad() const { return excessLoad_; }

pyvrp::Duration Trip::duration() const { return duration_; }

pyvrp::Duration Trip::serviceDuration() const { return service_; }

pyvrp::Duration Trip::timeWarp() const { return timeWarp_; }

pyvrp::Duration Trip::travelDuration() const { return travel_; }

pyvrp::Duration Trip::waitDuration() const { return wait_; }

pyvrp::Duration Trip::releaseTime() const { return release_; }

pyvrp::Cost Trip::prizes() const { return prizes_; }

std::pair<double, double> const &Trip::centroid() const { return centroid_; }

size_t Trip::vehicleType() const { return vehicleType_; }

size_t Trip::startDepot() const { return startDepot_; }

size_t Trip::endDepot() const { return endDepot_; }

bool Trip::isFeasible() const { return !hasExcessLoad() && !hasTimeWarp(); }

bool Trip::hasExcessLoad() const
{
    return std::any_of(excessLoad_.begin(),
                       excessLoad_.end(),
                       [](auto const excess) { return excess > 0; });
}

bool Trip::hasTimeWarp() const { return timeWarp_ > 0; }

bool Trip::operator==(Trip const &other) const
{
    // First compare simple attributes, since that's a quick and cheap check.
    // Only when these are the same we test if the visits are all equal.
    // clang-format off
    return distance_ == other.distance_
        && duration_ == other.duration_
        && timeWarp_ == other.timeWarp_
        && startDepot_ == other.startDepot_  // might vary between trips
        && endDepot_ == other.endDepot_      // might vary between trips
        && vehicleType_ == other.vehicleType_
        && visits_ == other.visits_;
    // clang-format on
}
