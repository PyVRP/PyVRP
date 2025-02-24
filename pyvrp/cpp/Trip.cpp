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
    // TODO determine start/end locations, initial state using after argument.

    // auto const &vehType = data.vehicleType(vehicleType_);

    // DurationSegment ds = {vehType, vehType.startLate};

    // std::vector<LoadSegment> loadSegments(data.numLoadDimensions());
    // for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
    //     loadSegments[dim] = {vehType, dim};

    // auto const &distances = data.distanceMatrix(vehType.profile);
    // auto const &durations = data.durationMatrix(vehType.profile);

    // for (size_t prevClient = startDepot_; auto const client : visits_)
    // {
    //     ProblemData::Client const &clientData = data.location(client);

    //     distance_ += distances(prevClient, client);
    //     service_ += clientData.serviceDuration;
    //     prizes_ += clientData.prize;

    //     auto const edgeDuration = durations(prevClient, client);
    //     travel_ += edgeDuration;
    //     ds = DurationSegment::merge(edgeDuration, ds, {clientData});

    //     centroid_.first += static_cast<double>(clientData.x) / size();
    //     centroid_.second += static_cast<double>(clientData.y) / size();

    //     for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
    //     {
    //         auto const clientLs = LoadSegment(clientData, dim);
    //         loadSegments[dim] = LoadSegment::merge(loadSegments[dim],
    //         clientLs);
    //     }

    //     prevClient = client;
    // }

    // auto const last = visits_.empty() ? startDepot_ : visits_.back();
    // distance_ += distances(last, endDepot_);
    // travel_ += durations(last, endDepot_);

    // delivery_.reserve(data.numLoadDimensions());
    // pickup_.reserve(data.numLoadDimensions());
    // excessLoad_.reserve(data.numLoadDimensions());
    // for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
    // {
    //     delivery_.push_back(loadSegments[dim].delivery());
    //     pickup_.push_back(loadSegments[dim].pickup());
    //     excessLoad_.push_back(std::max<Load>(
    //         loadSegments[dim].load() - vehType.capacity[dim], 0));
    // }

    // DurationSegment endDS(vehType, vehType.twLate);
    // ds = DurationSegment::merge(durations(last, endDepot_), ds, endDS);
    // duration_ = ds.duration();
    // startTime_ = ds.twEarly();
    // slack_ = ds.twLate() - ds.twEarly();
    // timeWarp_ = ds.timeWarp(vehType.maxDuration);
    // release_ = ds.releaseTime();
}
