#include "ProblemData.h"

#include <cassert>
#include <cstring>
#include <numeric>
#include <stdexcept>

using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Matrix;
using pyvrp::ProblemData;

namespace
{
// Small local helper for what is essentially strdup() from the C23 standard,
// which my compiler does not (yet) have. See here for the actual recipe:
// https://stackoverflow.com/a/252802/4316405 (modified to use new instead of
// malloc).
static char *duplicate(char const *src)
{
    char *dst = new char[std::strlen(src) + 1];  // space for src + null
    std::strcpy(dst, src);
    return dst;
}
}  // namespace

ProblemData::Client::Client(Coordinate x,
                            Coordinate y,
                            Load delivery,
                            Load pickup,
                            Duration serviceDuration,
                            Duration twEarly,
                            Duration twLate,
                            Duration releaseTime,
                            Cost prize,
                            bool required,
                            std::optional<size_t> group,
                            char const *name)
    : x(x),
      y(y),
      delivery(delivery),
      pickup(pickup),
      serviceDuration(serviceDuration),
      twEarly(twEarly),
      twLate(twLate),
      releaseTime(releaseTime),
      prize(prize),
      required(required),
      group(group),
      name(duplicate(name))
{
    if (delivery < 0)
        throw std::invalid_argument("delivery amount must be >= 0.");

    if (pickup < 0)
        throw std::invalid_argument("pickup amount must be >= 0.");

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
      delivery(client.delivery),
      pickup(client.pickup),
      serviceDuration(client.serviceDuration),
      twEarly(client.twEarly),
      twLate(client.twLate),
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
      delivery(client.delivery),
      pickup(client.pickup),
      serviceDuration(client.serviceDuration),
      twEarly(client.twEarly),
      twLate(client.twLate),
      releaseTime(client.releaseTime),
      prize(client.prize),
      required(client.required),
      group(client.group),
      name(client.name)  // we can steal
{
    client.name = nullptr;  // stolen
}

ProblemData::Client::~Client() { delete[] name; }

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
                          Duration twEarly,
                          Duration twLate,
                          char const *name)
    : x(x), y(y), twEarly(twEarly), twLate(twLate), name(duplicate(name))
{
    if (twEarly > twLate)
        throw std::invalid_argument("tw_early must be <= tw_late.");

    if (twEarly < 0)
        throw std::invalid_argument("tw_early must be >= 0.");
}

ProblemData::Depot::Depot(Depot const &depot)
    : x(depot.x),
      y(depot.y),
      twEarly(depot.twEarly),
      twLate(depot.twLate),
      name(duplicate(depot.name))
{
}

ProblemData::Depot::Depot(Depot &&depot)
    : x(depot.x),
      y(depot.y),
      twEarly(depot.twEarly),
      twLate(depot.twLate),
      name(depot.name)  // we can steal
{
    depot.name = nullptr;  // stolen
}

ProblemData::Depot::~Depot() { delete[] name; }

ProblemData::VehicleType::VehicleType(size_t numAvailable,
                                      Load capacity,
                                      size_t depot,
                                      Cost fixedCost,
                                      Duration twEarly,
                                      Duration twLate,
                                      Duration maxDuration,
                                      Distance maxDistance,
                                      char const *name)
    : numAvailable(numAvailable),
      depot(depot),
      capacity(capacity),
      fixedCost(fixedCost),
      twEarly(twEarly),
      twLate(twLate),
      maxDuration(maxDuration),
      maxDistance(maxDistance),
      name(duplicate(name))
{
    if (numAvailable == 0)
        throw std::invalid_argument("num_available must be > 0.");

    if (capacity < 0)
        throw std::invalid_argument("capacity must be >= 0.");

    if (fixedCost < 0)
        throw std::invalid_argument("fixed_cost must be >= 0.");

    if (twEarly > twLate)
        throw std::invalid_argument("tw_early must be <= tw_late.");

    if (twEarly < 0)
        throw std::invalid_argument("tw_early must be >= 0.");

    if (maxDuration < 0)
        throw std::invalid_argument("max_duration must be >= 0.");

    if (maxDistance < 0)
        throw std::invalid_argument("max_distance must be >= 0.");
}

ProblemData::VehicleType::VehicleType(VehicleType const &vehicleType)
    : numAvailable(vehicleType.numAvailable),
      depot(vehicleType.depot),
      capacity(vehicleType.capacity),
      fixedCost(vehicleType.fixedCost),
      twEarly(vehicleType.twEarly),
      twLate(vehicleType.twLate),
      maxDuration(vehicleType.maxDuration),
      maxDistance(vehicleType.maxDistance),
      name(duplicate(vehicleType.name))
{
}

ProblemData::VehicleType::VehicleType(VehicleType &&vehicleType)
    : numAvailable(vehicleType.numAvailable),
      depot(vehicleType.depot),
      capacity(vehicleType.capacity),
      fixedCost(vehicleType.fixedCost),
      twEarly(vehicleType.twEarly),
      twLate(vehicleType.twLate),
      maxDuration(vehicleType.maxDuration),
      maxDistance(vehicleType.maxDistance),
      name(vehicleType.name)  // we can steal
{
    vehicleType.name = nullptr;  // stolen
}

ProblemData::VehicleType::~VehicleType() { delete[] name; }

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

Distance ProblemData::dist(size_t first, size_t second) const
{
    return dist_(first, second);
}

Duration ProblemData::duration(size_t first, size_t second) const
{
    return dur_(first, second);
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

void ProblemData::validate() const
{
    // Client checks.
    for (size_t idx = numDepots(); idx != numLocations(); ++idx)
    {
        ProblemData::Client const &client = location(idx);
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
    for (auto const &vehicleType : vehicleTypes_)
        if (vehicleType.depot >= numDepots())
            throw std::out_of_range("Vehicle type has invalid depot.");

    // Matrix checks.
    if (dist_.numRows() != numLocations() || dist_.numCols() != numLocations())
        throw std::invalid_argument("Distance matrix shape does not match the "
                                    "problem size.");

    if (dur_.numRows() != numLocations() || dur_.numCols() != numLocations())
        throw std::invalid_argument("Duration matrix shape does not match the "
                                    "problem size.");

    for (size_t idx = 0; idx != numLocations(); ++idx)
    {
        if (dist_(idx, idx) != 0)
            throw std::invalid_argument("Distance matrix diagonal must be 0.");

        if (dur_(idx, idx) != 0)
            throw std::invalid_argument("Duration matrix diagonal must be 0.");
    }
}

ProblemData
ProblemData::replace(std::optional<std::vector<Client>> &clients,
                     std::optional<std::vector<Depot>> &depots,
                     std::optional<std::vector<VehicleType>> &vehicleTypes,
                     std::optional<Matrix<Distance>> &distMat,
                     std::optional<Matrix<Duration>> &durMat,
                     std::optional<std::vector<ClientGroup>> &groups)
{
    return ProblemData(clients.value_or(clients_),
                       depots.value_or(depots_),
                       vehicleTypes.value_or(vehicleTypes_),
                       distMat.value_or(dist_),
                       durMat.value_or(dur_),
                       groups.value_or(groups_));
}

ProblemData::ProblemData(std::vector<Client> clients,
                         std::vector<Depot> depots,
                         std::vector<VehicleType> vehicleTypes,
                         Matrix<Distance> distMat,
                         Matrix<Duration> durMat,
                         std::vector<ClientGroup> groups)
    : centroid_({0, 0}),
      dist_(std::move(distMat)),
      dur_(std::move(durMat)),
      clients_(std::move(clients)),
      depots_(std::move(depots)),
      vehicleTypes_(std::move(vehicleTypes)),
      groups_(std::move(groups)),
      numVehicles_(std::accumulate(vehicleTypes_.begin(),
                                   vehicleTypes_.end(),
                                   0,
                                   [](auto sum, VehicleType const &type)
                                   { return sum + type.numAvailable; }))
{
    for (auto const &client : clients_)
    {
        centroid_.first += static_cast<double>(client.x) / numClients();
        centroid_.second += static_cast<double>(client.y) / numClients();
    }

    validate();
}
