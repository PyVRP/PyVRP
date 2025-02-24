#include "Trip.h"
#include "DurationSegment.h"
#include "LoadSegment.h"

using pyvrp::Trip;

Trip::Trip([[maybe_unused]] ProblemData const &data,
           Visits visits,
           size_t const vehicleType,
           TripDelimiter start,
           TripDelimiter end,
           [[maybe_unused]] Trip const *after)
    : visits_(std::move(visits)),
      vehicleType_(vehicleType),
      start_(std::move(start)),
      end_(std::move(end))
{
    // TODO determine initial state using after argument.

    size_t const startDepot
        = std::holds_alternative<Depot const *>(start_)
              ? std::distance(data.depots().data(),
                              std::get<Depot const *>(start_))
              : std::get<Reload const *>(start_)->depot;

    if (startDepot >= data.numDepots())
        throw std::invalid_argument("Start depot not understood.");

    size_t const endDepot = std::holds_alternative<Depot const *>(end_)
                                ? std::distance(data.depots().data(),
                                                std::get<Depot const *>(end_))
                                : std::get<Reload const *>(end_)->depot;

    if (endDepot >= data.numDepots())
        throw std::invalid_argument("End depot not understood");

    auto const &vehType = data.vehicleType(vehicleType_);

    DurationSegment ds = {vehType, vehType.startLate};

    std::vector<LoadSegment> loadSegments(data.numLoadDimensions());
    for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
        loadSegments[dim] = {vehType, dim};

    auto const &distances = data.distanceMatrix(vehType.profile);
    auto const &durations = data.durationMatrix(vehType.profile);

    for (size_t prevClient = startDepot; auto const client : visits_)
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

    auto const last = visits_.empty() ? startDepot : visits_.back();
    distance_ += distances(last, endDepot);
    travel_ += durations(last, endDepot);

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

    DurationSegment endDS(vehType, vehType.twLate);
    ds = DurationSegment::merge(durations(last, endDepot), ds, endDS);
    duration_ = ds.duration();
    startTime_ = ds.twEarly();
    slack_ = ds.twLate() - ds.twEarly();
    timeWarp_ = ds.timeWarp(vehType.maxDuration);
    release_ = ds.releaseTime();
}

bool Trip::empty() const { return visits_.empty(); }

size_t Trip::size() const { return visits_.size(); }
