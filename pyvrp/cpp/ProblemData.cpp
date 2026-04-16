#include "ProblemData.h"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <stdexcept>

using pyvrp::Client;
using pyvrp::ClientGroup;
using pyvrp::Depot;
using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Load;
using pyvrp::Location;
using pyvrp::Matrix;
using pyvrp::ProblemData;
using pyvrp::VehicleType;

namespace
{
// Small helper that determines if the time windows of a and b overlap.
bool hasTimeOverlap(auto const &a, auto const &b)
{
    // See https://stackoverflow.com/a/325964/4316405.
    return a.twEarly <= b.twLate && a.twLate >= b.twEarly;
}

bool hasTimeWindow(auto const &arg)
{
    auto const hasTw = arg.twEarly != 0
                       || arg.twLate != std::numeric_limits<Duration>::max();

    if constexpr (requires { arg.startLate; })
        return hasTw || arg.startLate != std::numeric_limits<Duration>::max();

    return hasTw;
}
}  // namespace

std::vector<Location> const &ProblemData::locations() const
{
    return locations_;
}

std::vector<Client> const &ProblemData::clients() const { return clients_; }

std::vector<Depot> const &ProblemData::depots() const { return depots_; }

std::vector<ClientGroup> const &ProblemData::groups() const { return groups_; }

std::vector<VehicleType> const &ProblemData::vehicleTypes() const
{
    return vehicleTypes_;
}

std::vector<Matrix<Distance>> const &ProblemData::distanceMatrices() const
{
    return dists_;
}

std::vector<Matrix<Duration>> const &ProblemData::durationMatrices() const
{
    return durs_;
}

Location const &ProblemData::location(size_t location) const
{
    assert(location < numLocations());
    return locations_[location];
}

ClientGroup const &ProblemData::group(size_t group) const
{
    assert(group < groups_.size());
    return groups_[group];
}

VehicleType const &ProblemData::vehicleType(size_t vehicleType) const
{
    assert(vehicleType < vehicleTypes_.size());
    return vehicleTypes_[vehicleType];
}

size_t ProblemData::numClients() const { return clients_.size(); }

size_t ProblemData::numDepots() const { return depots_.size(); }

size_t ProblemData::numGroups() const { return groups_.size(); }

size_t ProblemData::numLocations() const { return locations_.size(); }

size_t ProblemData::numVehicleTypes() const { return vehicleTypes_.size(); }

size_t ProblemData::numVehicles() const { return numVehicles_; }

size_t ProblemData::numProfiles() const
{
    assert(dists_.size() == durs_.size());
    return dists_.size();
}

size_t ProblemData::numLoadDimensions() const { return numLoadDimensions_; }

void ProblemData::validate() const
{
    // Client checks.
    for (size_t idx = 0; idx != numClients(); ++idx)
    {
        auto const &client = clients_[idx];

        if (client.location >= numLocations())
            throw std::out_of_range("Client references invalid location.");

        if (client.delivery.size() != numLoadDimensions_)
        {
            auto const *msg = "Client has inconsistent delivery size.";
            throw std::invalid_argument(msg);
        }

        if (client.pickup.size() != numLoadDimensions_)
        {
            auto const *msg = "Client has inconsistent pickup size.";
            throw std::invalid_argument(msg);
        }

        if (!client.group)
            continue;

        if (*client.group >= numGroups())
            throw std::out_of_range("Client references invalid group.");

        auto const &group = groups_[*client.group];
        if (std::find(group.begin(), group.end(), idx) == group.end())
        {
            auto const *msg = "Client not in the group it references.";
            throw std::invalid_argument(msg);
        }

        if (client.required && group.mutuallyExclusive)
        {
            auto const *msg = "Required client in mutually exclusive group.";
            throw std::invalid_argument(msg);
        }
    }

    // Depot checks.
    if (depots_.empty())
        throw std::invalid_argument("Expected at least one depot.");

    for (auto const &depot : depots_)
        if (depot.location >= numLocations())
            throw std::out_of_range("Depot references invalid location.");

    // Group checks.
    for (size_t idx = 0; idx != numGroups(); ++idx)
    {
        auto const &group = groups_[idx];

        if (group.empty())
            throw std::invalid_argument("Empty client group not understood.");

        for (auto const client : group)
        {
            if (client >= numClients())
                throw std::out_of_range("Group references invalid client.");

            auto const &clientData = clients_[client];
            if (!clientData.group || *clientData.group != idx)
            {
                auto const *msg = "Group references client not in group.";
                throw std::invalid_argument(msg);
            }
        }
    }

    // Vehicle type checks.
    if (vehicleTypes_.empty())
        throw std::invalid_argument("Expected at least one vehicle type.");

    for (auto const &vehicleType : vehicleTypes_)
    {
        if (vehicleType.capacity.size() != numLoadDimensions_)
        {
            auto const *msg = "Vehicle type has inconsistent capacity size.";
            throw std::invalid_argument(msg);
        }

        if (vehicleType.startDepot >= numDepots())
            throw std::out_of_range("Vehicle type has invalid start depot.");

        if (vehicleType.endDepot >= numDepots())
            throw std::out_of_range("Vehicle type has invalid end depot.");

        if (vehicleType.profile >= dists_.size())
            throw std::out_of_range("Vehicle type has invalid profile.");

        if (!hasTimeOverlap(depots_[vehicleType.startDepot], vehicleType))
            throw std::invalid_argument("Vehicle and its start depot have no "
                                        "overlapping time windows.");

        if (!hasTimeOverlap(depots_[vehicleType.endDepot], vehicleType))
            throw std::invalid_argument("Vehicle and its end depot have no "
                                        "overlapping time windows.");

        for (auto const depot : vehicleType.reloadDepots)
            if (depot >= numDepots())
                throw std::out_of_range("Vehicle has invalid reload depot.");
    }

    // Matrix checks.
    if (dists_.empty() || durs_.empty())
        throw std::invalid_argument("Need at least one distance and duration "
                                    "matrix.");

    if (dists_.size() != durs_.size())
        throw std::invalid_argument("Inconsistent number of distance and "
                                    "duration matrices.");

    for (size_t idx = 0; idx != dists_.size(); ++idx)
    {
        auto const numLocs = numLocations();
        auto const &dist = dists_[idx];
        auto const &dur = durs_[idx];

        if (dist.numRows() != numLocs || dist.numCols() != numLocs)
            throw std::invalid_argument("Distance matrix shape does not match "
                                        "the problem size.");

        if (dur.numRows() != numLocs || dur.numCols() != numLocs)
            throw std::invalid_argument("Duration matrix shape does not match "
                                        "the problem size.");

        for (size_t loc = 0; loc != numLocs; ++loc)
        {
            if (dist(loc, loc) != 0)
                throw std::invalid_argument("Distance matrix diagonals must be "
                                            "all zero.");

            if (dur(loc, loc) != 0)
                throw std::invalid_argument("Duration matrix diagonals must be "
                                            "all zero.");
        }
    }
}

ProblemData
ProblemData::replace(std::optional<std::vector<Location>> &locations,
                     std::optional<std::vector<Client>> &clients,
                     std::optional<std::vector<Depot>> &depots,
                     std::optional<std::vector<VehicleType>> &vehicleTypes,
                     std::optional<std::vector<Matrix<Distance>>> &distMats,
                     std::optional<std::vector<Matrix<Duration>>> &durMats,
                     std::optional<std::vector<ClientGroup>> &groups) const
{
    return {locations.value_or(locations_),
            clients.value_or(clients_),
            depots.value_or(depots_),
            vehicleTypes.value_or(vehicleTypes_),
            distMats.value_or(dists_),
            durMats.value_or(durs_),
            groups.value_or(groups_)};
}

ProblemData::ProblemData(std::vector<Location> locations,
                         std::vector<Client> clients,
                         std::vector<Depot> depots,
                         std::vector<VehicleType> vehicleTypes,
                         std::vector<Matrix<Distance>> distMats,
                         std::vector<Matrix<Duration>> durMats,
                         std::vector<ClientGroup> groups)
    : dists_(std::move(distMats)),
      durs_(std::move(durMats)),
      locations_(std::move(locations)),
      clients_(std::move(clients)),
      depots_(std::move(depots)),
      vehicleTypes_(std::move(vehicleTypes)),
      groups_(std::move(groups)),
      numVehicles_(std::accumulate(vehicleTypes_.begin(),
                                   vehicleTypes_.end(),
                                   0,
                                   [](auto sum, VehicleType const &type)
                                   { return sum + type.numAvailable; })),
      numLoadDimensions_(
          clients_.empty()
              // If there are no clients we look at the vehicle types. If both
              // are empty we default to 0. Clients have pickups and deliveries
              // but the client constructor already ensures those are of equal
              // size (within a single client).
              ? (vehicleTypes_.empty() ? 0 : vehicleTypes_[0].capacity.size())
              : clients_[0].delivery.size()),
      hasTimeWindows_(
          std::any_of(clients_.begin(), clients_.end(), hasTimeWindow<Client>)
          || std::any_of(depots_.begin(), depots_.end(), hasTimeWindow<Depot>)
          || std::any_of(vehicleTypes_.begin(),
                         vehicleTypes_.end(),
                         hasTimeWindow<VehicleType>))
{
    validate();
}
