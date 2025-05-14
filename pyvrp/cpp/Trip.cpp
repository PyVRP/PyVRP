#include "Trip.h"
#include "LoadSegment.h"

#include <algorithm>
#include <fstream>

using pyvrp::Cost;
using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Load;
using pyvrp::ProblemData;
using pyvrp::Trip;

namespace
{
// Returns whether given vehicle type can start a trip from the given depot.
bool canStartAt(ProblemData::VehicleType const &vehType, size_t depot)
{
    auto const &reloads = vehType.reloadDepots;
    return depot == vehType.startDepot
           || std::find(reloads.begin(), reloads.end(), depot) != reloads.end();
}

// Returns whether given vehicle type can end a trip at the given depot.
bool canEndAt(ProblemData::VehicleType const &vehType, size_t depot)
{
    auto const &reloads = vehType.reloadDepots;
    return depot == vehType.endDepot
           || std::find(reloads.begin(), reloads.end(), depot) != reloads.end();
}
}  // namespace

Trip::Trip(ProblemData const &data,
           Visits visits,
           size_t vehicleType,
           std::optional<size_t> startDepot,
           std::optional<size_t> endDepot)
    : visits_(std::move(visits)),
      delivery_(data.numLoadDimensions(), 0),
      pickup_(data.numLoadDimensions(), 0),
      load_(data.numLoadDimensions(), 0),
      excessLoad_(data.numLoadDimensions(), 0),
      vehicleType_(vehicleType)
{
    auto const &vehData = data.vehicleType(vehicleType_);
    startDepot_ = startDepot.value_or(vehData.startDepot);
    endDepot_ = endDepot.value_or(vehData.endDepot);

    if (!canStartAt(vehData, startDepot_))
        throw std::invalid_argument("Vehicle cannot start from start_depot.");

    if (!canEndAt(vehData, endDepot_))
        throw std::invalid_argument("Vehicle cannot end at end_depot.");

    for (auto const client : visits_)
        if (client < data.numDepots() || client >= data.numLocations())
        {
            std::ostringstream msg;
            msg << "Client " << client << " is not understood.";
            throw std::invalid_argument(msg.str());
        }

    auto const &distances = data.distanceMatrix(vehData.profile);
    auto const &durations = data.durationMatrix(vehData.profile);

    std::vector<LoadSegment> loadSegments(data.numLoadDimensions());
    for (size_t prevClient = startDepot_; auto const client : visits_)
    {
        distance_ += distances(prevClient, client);
        travel_ += durations(prevClient, client);

        ProblemData::Client const &clientData = data.location(client);

        service_ += clientData.serviceDuration;
        release_ = std::max(release_, clientData.releaseTime);
        prizes_ += clientData.prize;

        centroid_.first += static_cast<double>(clientData.x) / size();
        centroid_.second += static_cast<double>(clientData.y) / size();

        for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
            loadSegments[dim]
                = LoadSegment::merge(loadSegments[dim], {clientData, dim});

        prevClient = client;
    }

    auto const last = empty() ? startDepot_ : visits_.back();
    distance_ += distances(last, endDepot_);
    travel_ += durations(last, endDepot_);

    for (size_t dim = 0; dim != data.numLoadDimensions(); ++dim)
    {
        delivery_[dim] = loadSegments[dim].delivery();
        pickup_[dim] = loadSegments[dim].pickup();
        load_[dim] = loadSegments[dim].load();
        excessLoad_[dim] = loadSegments[dim].excessLoad(vehData.capacity[dim]);
    }
}

Trip::Trip(Visits visits,
           Distance distance,
           std::vector<Load> delivery,
           std::vector<Load> pickup,
           std::vector<Load> load,
           std::vector<Load> excessLoad,
           Duration travel,
           Duration service,
           Duration release,
           Cost prizes,
           std::pair<double, double> centroid,
           size_t vehicleType,
           size_t startDepot,
           size_t endDepot)
    : visits_(std::move(visits)),
      distance_(distance),
      delivery_(std::move(delivery)),
      pickup_(std::move(pickup)),
      load_(std::move(load)),
      excessLoad_(std::move(excessLoad)),
      travel_(travel),
      service_(service),
      release_(release),
      prizes_(prizes),
      centroid_(centroid),
      vehicleType_(vehicleType),
      startDepot_(startDepot),
      endDepot_(endDepot)
{
}

bool Trip::empty() const { return visits_.empty(); }

size_t Trip::size() const { return visits_.size(); }

Trip::Client Trip::operator[](size_t idx) const { return visits_[idx]; }

Trip::Visits::const_iterator Trip::begin() const { return visits_.begin(); }
Trip::Visits::const_iterator Trip::end() const { return visits_.end(); }

Trip::Visits::const_reverse_iterator Trip::rbegin() const
{
    return visits_.rbegin();
}
Trip::Visits::const_reverse_iterator Trip::rend() const
{
    return visits_.rend();
}

Trip::Visits const &Trip::visits() const { return visits_; }

Distance Trip::distance() const { return distance_; }

std::vector<Load> const &Trip::delivery() const { return delivery_; }

std::vector<Load> const &Trip::pickup() const { return pickup_; }

std::vector<Load> const &Trip::load() const { return load_; }

std::vector<Load> const &Trip::excessLoad() const { return excessLoad_; }

Duration Trip::serviceDuration() const { return service_; }

Duration Trip::travelDuration() const { return travel_; }

Duration Trip::releaseTime() const { return release_; }

Cost Trip::prizes() const { return prizes_; }

std::pair<double, double> const &Trip::centroid() const { return centroid_; }

size_t Trip::vehicleType() const { return vehicleType_; }

size_t Trip::startDepot() const { return startDepot_; }

size_t Trip::endDepot() const { return endDepot_; }

bool Trip::hasExcessLoad() const
{
    return std::any_of(excessLoad_.begin(),
                       excessLoad_.end(),
                       [](auto const excess) { return excess > 0; });
}

bool Trip::operator==(Trip const &other) const
{
    // First compare simple attributes, since that's a quick and cheap check.
    // Only when these are the same we test if the visits are all equal.
    // clang-format off
    return distance_ == other.distance_
        && travel_ == other.travel_
        && service_ == other.service_
        && startDepot_ == other.startDepot_  // might vary between trips
        && endDepot_ == other.endDepot_      // might vary between trips
        && vehicleType_ == other.vehicleType_
        && visits_ == other.visits_;
    // clang-format on
}

std::ostream &operator<<(std::ostream &out, Trip const &trip)
{
    for (size_t idx = 0; idx != trip.size(); ++idx)
    {
        if (idx != 0)
            out << ' ';
        out << trip[idx];
    }

    return out;
}
