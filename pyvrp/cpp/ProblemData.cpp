#include "ProblemData.h"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <stdexcept>

using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Load;
using pyvrp::Matrix;
using pyvrp::ProblemData;

namespace
{
// Small local helper for what is essentially strdup() from the C23 standard,
// which my compiler does not (yet) have. See here for the actual recipe:
// https://stackoverflow.com/a/252802/4316405 (modified to use new instead of
// malloc). We do all this so we can use C-style strings, rather than C++'s
// std::string, which are much larger objects.
static char *duplicate(char const *src)
{
    char *dst = new char[std::strlen(src) + 1];  // space for src + null
    std::strcpy(dst, src);
    return dst;
}

// Pad vec1 with zeroes to the size of vec1 and vec2, whichever is larger.
auto &pad(auto &vec1, auto const &vec2)
{
    vec1.resize(std::max(vec1.size(), vec2.size()));
    return vec1;
}

bool isNegative(auto value) { return value < 0; }

// Small helper that determines if the time windows of a and b overlap.
bool hasTimeOverlap(auto const &a, auto const &b)
{
    // See https://stackoverflow.com/a/325964/4316405.
    return a.twEarly <= b.twLate && a.twLate >= b.twEarly;
}
}  // namespace

ProblemData::Client::Client(Coordinate x,
                            Coordinate y,
                            std::vector<Load> delivery,
                            std::vector<Load> pickup,
                            Duration serviceDuration,
                            Duration twEarly,
                            Duration twLate,
                            Duration releaseTime,
                            Cost prize,
                            bool required,
                            std::optional<size_t> group,
                            std::string name)
    : x(x),
      y(y),
      serviceDuration(serviceDuration),
      twEarly(twEarly),
      twLate(twLate),
      delivery(pad(delivery, pickup)),
      pickup(pad(pickup, delivery)),
      releaseTime(releaseTime),
      prize(prize),
      required(required),
      group(group),
      name(duplicate(name.data()))
{
    assert(delivery.size() == pickup.size());

    if (std::any_of(delivery.begin(), delivery.end(), isNegative<Load>))
        throw std::invalid_argument("delivery amounts must be >= 0.");

    if (std::any_of(pickup.begin(), pickup.end(), isNegative<Load>))
        throw std::invalid_argument("pickup amounts must be >= 0.");

    if (serviceDuration < 0)
        throw std::invalid_argument("service_duration must be >= 0.");

    if (twEarly > twLate)
        throw std::invalid_argument("tw_early must be <= tw_late.");

    if (twEarly < 0)
        throw std::invalid_argument("tw_early must be >= 0.");

    if (releaseTime > twLate)
        throw std::invalid_argument("release_time must be <= tw_late");

    if (releaseTime < 0)
        throw std::invalid_argument("release_time must be >= 0.");

    if (prize < 0)
        throw std::invalid_argument("prize must be >= 0.");
}

ProblemData::Client::Client(Client const &client)
    : x(client.x),
      y(client.y),
      serviceDuration(client.serviceDuration),
      twEarly(client.twEarly),
      twLate(client.twLate),
      delivery(client.delivery),
      pickup(client.pickup),
      releaseTime(client.releaseTime),
      prize(client.prize),
      required(client.required),
      group(client.group),
      name(duplicate(client.name))
{
}

ProblemData::Client::Client(Client &&client)
    : x(client.x),
      y(client.y),
      serviceDuration(client.serviceDuration),
      twEarly(client.twEarly),
      twLate(client.twLate),
      delivery(client.delivery),
      pickup(client.pickup),
      releaseTime(client.releaseTime),
      prize(client.prize),
      required(client.required),
      group(client.group),
      name(client.name)  // we can steal
{
    client.name = nullptr;  // stolen
}

ProblemData::Client::~Client() { delete[] name; }

bool ProblemData::Client::operator==(Client const &other) const
{
    // clang-format off
    return x == other.x
        && y == other.y
        && delivery == other.delivery
        && pickup == other.pickup
        && serviceDuration == other.serviceDuration
        && twEarly == other.twEarly
        && twLate == other.twLate
        && releaseTime == other.releaseTime
        && prize == other.prize
        && required == other.required
        && group == other.group
        && std::strcmp(name, other.name) == 0;
    // clang-format on
}

ProblemData::ClientGroup::ClientGroup(std::vector<size_t> clients,
                                      bool required)
    : required(required)
{
    for (auto const client : clients)
        addClient(client);
}

bool ProblemData::ClientGroup::empty() const { return clients_.empty(); }

size_t ProblemData::ClientGroup::size() const { return clients_.size(); }

std::vector<size_t>::const_iterator ProblemData::ClientGroup::begin() const
{
    return clients_.begin();
}

std::vector<size_t>::const_iterator ProblemData::ClientGroup::end() const
{
    return clients_.end();
}

std::vector<size_t> const &ProblemData::ClientGroup::clients() const
{
    return clients_;
}

void ProblemData::ClientGroup::addClient(size_t client)
{
    if (std::find(clients_.begin(), clients_.end(), client) != clients_.end())
        throw std::invalid_argument("Client already in group.");

    clients_.push_back(client);
}

void ProblemData::ClientGroup::clear() { clients_.clear(); }

ProblemData::Depot::Depot(Coordinate x,
                          Coordinate y,
                          Duration serviceDuration,
                          Duration twEarly,
                          Duration twLate,
                          std::string name)
    : x(x),
      y(y),
      serviceDuration(serviceDuration),
      twEarly(twEarly),
      twLate(twLate),
      name(duplicate(name.data()))
{
    if (serviceDuration < 0)
        throw std::invalid_argument("service_duration must be >= 0.");

    if (twEarly > twLate)
        throw std::invalid_argument("tw_early must be <= tw_late.");

    if (twEarly < 0)
        throw std::invalid_argument("tw_early must be >= 0.");
}

ProblemData::Depot::Depot(Depot const &depot)
    : x(depot.x),
      y(depot.y),
      serviceDuration(depot.serviceDuration),
      twEarly(depot.twEarly),
      twLate(depot.twLate),
      name(duplicate(depot.name))
{
}

ProblemData::Depot::Depot(Depot &&depot)
    : x(depot.x),
      y(depot.y),
      serviceDuration(depot.serviceDuration),
      twEarly(depot.twEarly),
      twLate(depot.twLate),
      name(depot.name)  // we can steal
{
    depot.name = nullptr;  // stolen
}

ProblemData::Depot::~Depot() { delete[] name; }

bool ProblemData::Depot::operator==(Depot const &other) const
{
    // clang-format off
    return x == other.x 
        && y == other.y
        && serviceDuration == other.serviceDuration
        && twEarly == other.twEarly
        && twLate == other.twLate
        && std::strcmp(name, other.name) == 0;
    // clang-format on
}

ProblemData::VehicleType::VehicleType(size_t numAvailable,
                                      std::vector<Load> capacity,
                                      size_t startDepot,
                                      size_t endDepot,
                                      Cost fixedCost,
                                      Duration twEarly,
                                      Duration twLate,
                                      Duration maxDuration,
                                      Distance maxDistance,
                                      Cost unitDistanceCost,
                                      Cost unitDurationCost,
                                      size_t profile,
                                      std::optional<Duration> startLate,
                                      std::vector<Load> initialLoad,
                                      std::string name)
    : numAvailable(numAvailable),
      startDepot(startDepot),
      endDepot(endDepot),
      capacity(pad(capacity, initialLoad)),
      twEarly(twEarly),
      twLate(twLate),
      maxDuration(maxDuration),
      maxDistance(maxDistance),
      fixedCost(fixedCost),
      unitDistanceCost(unitDistanceCost),
      unitDurationCost(unitDurationCost),
      profile(profile),
      startLate(startLate.value_or(twLate)),
      initialLoad(pad(initialLoad, capacity)),
      name(duplicate(name.data()))
{
    if (numAvailable == 0)
        throw std::invalid_argument("num_available must be > 0.");

    if (std::any_of(capacity.begin(), capacity.end(), isNegative<Load>))
        throw std::invalid_argument("capacity amounts must be >= 0.");

    if (twEarly > this->startLate)
        throw std::invalid_argument("tw_early must be <= start_late.");

    if (this->startLate > twLate)
        throw std::invalid_argument("start_late must be <= tw_late.");

    if (twEarly < 0)
        throw std::invalid_argument("tw_early must be >= 0.");

    if (maxDuration < 0)
        throw std::invalid_argument("max_duration must be >= 0.");

    if (maxDistance < 0)
        throw std::invalid_argument("max_distance must be >= 0.");

    if (fixedCost < 0)
        throw std::invalid_argument("fixed_cost must be >= 0.");

    if (unitDistanceCost < 0)
        throw std::invalid_argument("unit_distance_cost must be >= 0.");

    if (unitDurationCost < 0)
        throw std::invalid_argument("unit_duration_cost must be >= 0.");

    if (std::any_of(initialLoad.begin(), initialLoad.end(), isNegative<Load>))
        throw std::invalid_argument("initial load amounts must be >= 0.");

    for (size_t dim = 0; dim != initialLoad.size(); ++dim)
        if (initialLoad[dim] > capacity[dim])
            throw std::invalid_argument("initial load exceeds capacity.");
}

ProblemData::VehicleType::VehicleType(VehicleType const &vehicleType)
    : numAvailable(vehicleType.numAvailable),
      startDepot(vehicleType.startDepot),
      endDepot(vehicleType.endDepot),
      capacity(vehicleType.capacity),
      twEarly(vehicleType.twEarly),
      twLate(vehicleType.twLate),
      maxDuration(vehicleType.maxDuration),
      maxDistance(vehicleType.maxDistance),
      fixedCost(vehicleType.fixedCost),
      unitDistanceCost(vehicleType.unitDistanceCost),
      unitDurationCost(vehicleType.unitDurationCost),
      profile(vehicleType.profile),
      startLate(vehicleType.startLate),
      initialLoad(vehicleType.initialLoad),
      name(duplicate(vehicleType.name))
{
}

ProblemData::VehicleType::VehicleType(VehicleType &&vehicleType)
    : numAvailable(vehicleType.numAvailable),
      startDepot(vehicleType.startDepot),
      endDepot(vehicleType.endDepot),
      capacity(vehicleType.capacity),
      twEarly(vehicleType.twEarly),
      twLate(vehicleType.twLate),
      maxDuration(vehicleType.maxDuration),
      maxDistance(vehicleType.maxDistance),
      fixedCost(vehicleType.fixedCost),
      unitDistanceCost(vehicleType.unitDistanceCost),
      unitDurationCost(vehicleType.unitDurationCost),
      profile(vehicleType.profile),
      startLate(vehicleType.startLate),
      initialLoad(vehicleType.initialLoad),
      name(vehicleType.name)  // we can steal
{
    vehicleType.name = nullptr;  // stolen
}

ProblemData::VehicleType::~VehicleType() { delete[] name; }

ProblemData::VehicleType
ProblemData::VehicleType::replace(std::optional<size_t> numAvailable,
                                  std::optional<std::vector<Load>> capacity,
                                  std::optional<size_t> startDepot,
                                  std::optional<size_t> endDepot,
                                  std::optional<Cost> fixedCost,
                                  std::optional<Duration> twEarly,
                                  std::optional<Duration> twLate,
                                  std::optional<Duration> maxDuration,
                                  std::optional<Distance> maxDistance,
                                  std::optional<Cost> unitDistanceCost,
                                  std::optional<Cost> unitDurationCost,
                                  std::optional<size_t> profile,
                                  std::optional<Duration> startLate,
                                  std::optional<std::vector<Load>> initialLoad,
                                  std::optional<std::string> name) const
{
    return {numAvailable.value_or(this->numAvailable),
            capacity.value_or(this->capacity),
            startDepot.value_or(this->startDepot),
            endDepot.value_or(this->endDepot),
            fixedCost.value_or(this->fixedCost),
            twEarly.value_or(this->twEarly),
            twLate.value_or(this->twLate),
            maxDuration.value_or(this->maxDuration),
            maxDistance.value_or(this->maxDistance),
            unitDistanceCost.value_or(this->unitDistanceCost),
            unitDurationCost.value_or(this->unitDurationCost),
            profile.value_or(this->profile),
            startLate.value_or(this->startLate),
            initialLoad.value_or(this->initialLoad),
            name.value_or(this->name)};
}

bool ProblemData::VehicleType::operator==(VehicleType const &other) const
{
    // clang-format off
    return numAvailable == other.numAvailable
        && capacity == other.capacity
        && startDepot == other.startDepot
        && endDepot == other.endDepot
        && fixedCost == other.fixedCost
        && twEarly == other.twEarly
        && twLate == other.twLate
        && maxDuration == other.maxDuration
        && maxDistance == other.maxDistance
        && unitDistanceCost == other.unitDistanceCost
        && unitDurationCost == other.unitDurationCost
        && profile == other.profile
        && startLate == other.startLate
        && initialLoad == other.initialLoad
        && std::strcmp(name, other.name) == 0;
    // clang-format on
}

std::vector<ProblemData::Client> const &ProblemData::clients() const
{
    return clients_;
}

std::vector<ProblemData::Depot> const &ProblemData::depots() const
{
    return depots_;
}

std::vector<ProblemData::ClientGroup> const &ProblemData::groups() const
{
    return groups_;
}

std::vector<ProblemData::VehicleType> const &ProblemData::vehicleTypes() const
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

ProblemData::ClientGroup const &ProblemData::group(size_t group) const
{
    assert(group < groups_.size());
    return groups_[group];
}

ProblemData::VehicleType const &
ProblemData::vehicleType(size_t vehicleType) const
{
    assert(vehicleType < vehicleTypes_.size());
    return vehicleTypes_[vehicleType];
}

std::pair<double, double> const &ProblemData::centroid() const
{
    return centroid_;
}

size_t ProblemData::numClients() const { return clients_.size(); }

size_t ProblemData::numDepots() const { return depots_.size(); }

size_t ProblemData::numGroups() const { return groups_.size(); }

size_t ProblemData::numLocations() const { return numDepots() + numClients(); }

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
    for (size_t idx = numDepots(); idx != numLocations(); ++idx)
    {
        ProblemData::Client const &client = location(idx);

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

    // Group checks.
    for (size_t idx = 0; idx != numGroups(); ++idx)
    {
        auto const &group = groups_[idx];

        if (group.empty())
            throw std::invalid_argument("Empty client group not understood.");

        for (auto const client : group)
        {
            if (client < numDepots() || client >= numLocations())
                throw std::out_of_range("Group references invalid client.");

            ProblemData::Client const &clientData = location(client);
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
ProblemData::replace(std::optional<std::vector<Client>> &clients,
                     std::optional<std::vector<Depot>> &depots,
                     std::optional<std::vector<VehicleType>> &vehicleTypes,
                     std::optional<std::vector<Matrix<Distance>>> &distMats,
                     std::optional<std::vector<Matrix<Duration>>> &durMats,
                     std::optional<std::vector<ClientGroup>> &groups) const
{
    return {clients.value_or(clients_),
            depots.value_or(depots_),
            vehicleTypes.value_or(vehicleTypes_),
            distMats.value_or(dists_),
            durMats.value_or(durs_),
            groups.value_or(groups_)};
}

ProblemData::ProblemData(std::vector<Client> clients,
                         std::vector<Depot> depots,
                         std::vector<VehicleType> vehicleTypes,
                         std::vector<Matrix<Distance>> distMats,
                         std::vector<Matrix<Duration>> durMats,
                         std::vector<ClientGroup> groups)
    : centroid_({0, 0}),
      dists_(std::move(distMats)),
      durs_(std::move(durMats)),
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
              : clients_[0].delivery.size())
{
    for (auto const &client : clients_)
    {
        centroid_.first += static_cast<double>(client.x) / numClients();
        centroid_.second += static_cast<double>(client.y) / numClients();
    }

    validate();
}
