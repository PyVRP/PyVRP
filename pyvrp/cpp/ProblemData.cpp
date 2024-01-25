#include "ProblemData.h"

#include <cstring>
#include <numeric>

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
                            Load demand,
                            Duration serviceDuration,
                            Duration twEarly,
                            Duration twLate,
                            Duration releaseTime,
                            Cost prize,
                            bool required,
                            char const *name)
    : x(x),
      y(y),
      demand(demand),
      serviceDuration(serviceDuration),
      twEarly(twEarly),
      twLate(twLate),
      releaseTime(releaseTime),
      prize(prize),
      name(duplicate(name)),
      required(required)
{
    if (demand < 0)
        throw std::invalid_argument("demand must be >= 0.");

    if (serviceDuration < 0)
        throw std::invalid_argument("service_duration must be >= 0.");

    if (twEarly > twLate)
        throw std::invalid_argument("tw_early must be <= tw_late.");

    if (twEarly < 0)
        throw std::invalid_argument("tw_early must be >= 0.");

    if (prize < 0)
        throw std::invalid_argument("prize must be >= 0.");
}

ProblemData::Client::Client(Client const &client)
    : x(client.x),
      y(client.y),
      demand(client.demand),
      serviceDuration(client.serviceDuration),
      twEarly(client.twEarly),
      twLate(client.twLate),
      releaseTime(client.releaseTime),
      prize(client.prize),
      name(duplicate(client.name)),
      required(client.required)
{
}

ProblemData::Client::Client(Client &&client)
    : x(client.x),
      y(client.y),
      demand(client.demand),
      serviceDuration(client.serviceDuration),
      twEarly(client.twEarly),
      twLate(client.twLate),
      releaseTime(client.releaseTime),
      prize(client.prize),
      name(client.name),  // we can steal
      required(client.required)
{
    client.name = nullptr;  // stolen
}

ProblemData::Client::~Client() { delete[] name; }

ProblemData::VehicleType::VehicleType(size_t numAvailable,
                                      Load capacity,
                                      size_t depot,
                                      Cost fixedCost,
                                      std::optional<Duration> twEarly,
                                      std::optional<Duration> twLate,
                                      std::optional<Duration> maxDuration,
                                      char const *name)
    : numAvailable(numAvailable),
      depot(depot),
      capacity(capacity),
      fixedCost(fixedCost),
      twEarly(twEarly),
      twLate(twLate),
      name(duplicate(name)),
      maxDuration(maxDuration.value_or(std::numeric_limits<Duration>::max()))
{
    if (numAvailable == 0)
        throw std::invalid_argument("num_available must be > 0.");

    if (capacity < 0)
        throw std::invalid_argument("capacity must be >= 0.");

    if (fixedCost < 0)
        throw std::invalid_argument("fixed_cost must be >= 0.");

    if ((twEarly && !twLate) || (!twEarly && twLate))
        throw std::invalid_argument("Must pass either no shift time window,"
                                    " or both a start and end.");

    if (twEarly && twLate)
    {
        if (twEarly > twLate)
            throw std::invalid_argument("tw_early must be <= tw_late.");

        if (twEarly < 0)
            throw std::invalid_argument("tw_early must be >= 0.");
    }

    if (this->maxDuration < 0)
        throw std::invalid_argument("max_duration must be >= 0.");
}

ProblemData::VehicleType::VehicleType(VehicleType const &vehicleType)
    : numAvailable(vehicleType.numAvailable),
      depot(vehicleType.depot),
      capacity(vehicleType.capacity),
      fixedCost(vehicleType.fixedCost),
      twEarly(vehicleType.twEarly),
      twLate(vehicleType.twLate),
      name(duplicate(vehicleType.name)),
      maxDuration(vehicleType.maxDuration)
{
}

ProblemData::VehicleType::VehicleType(VehicleType &&vehicleType)
    : numAvailable(vehicleType.numAvailable),
      depot(vehicleType.depot),
      capacity(vehicleType.capacity),
      fixedCost(vehicleType.fixedCost),
      twEarly(vehicleType.twEarly),
      twLate(vehicleType.twLate),
      name(vehicleType.name),  // we can steal
      maxDuration(vehicleType.maxDuration)
{
    vehicleType.name = nullptr;  // stolen
}

ProblemData::VehicleType::~VehicleType() { delete[] name; }

std::vector<ProblemData::Client> const &ProblemData::clients() const
{
    return clients_;
}

std::vector<ProblemData::Client> const &ProblemData::depots() const
{
    return depots_;
}

std::pair<double, double> const &ProblemData::centroid() const
{
    return centroid_;
}

size_t ProblemData::numClients() const { return clients_.size(); }

size_t ProblemData::numDepots() const { return depots_.size(); }

size_t ProblemData::numLocations() const { return numDepots() + numClients(); }

size_t ProblemData::numVehicleTypes() const { return vehicleTypes_.size(); }

size_t ProblemData::numVehicles() const { return numVehicles_; }

ProblemData
ProblemData::replace(std::optional<std::vector<Client>> &clients,
                     std::optional<std::vector<Client>> &depots,
                     std::optional<std::vector<VehicleType>> &vehicleTypes,
                     std::optional<Matrix<Distance>> &distMat,
                     std::optional<Matrix<Duration>> &durMat)
{
    return ProblemData(clients.value_or(clients_),
                       depots.value_or(depots_),
                       vehicleTypes.value_or(vehicleTypes_),
                       distMat.value_or(dist_),
                       durMat.value_or(dur_));
}

ProblemData::ProblemData(std::vector<Client> const &clients,
                         std::vector<Client> const &depots,
                         std::vector<VehicleType> const &vehicleTypes,
                         Matrix<Distance> distMat,
                         Matrix<Duration> durMat)
    : centroid_({0, 0}),
      dist_(std::move(distMat)),
      dur_(std::move(durMat)),
      clients_(clients),
      depots_(depots),
      vehicleTypes_(vehicleTypes),
      numVehicles_(std::accumulate(vehicleTypes.begin(),
                                   vehicleTypes.end(),
                                   0,
                                   [](auto sum, VehicleType const &type) {
                                       return sum + type.numAvailable;
                                   }))
{
    if (depots.empty())
        throw std::invalid_argument("Expected at least one depot!");

    if (dist_.numRows() != numLocations() || dist_.numCols() != numLocations())
        throw std::invalid_argument("Distance matrix shape does not match the "
                                    "problem size.");

    if (dur_.numRows() != numLocations() || dur_.numCols() != numLocations())
        throw std::invalid_argument("Duration matrix shape does not match the "
                                    "problem size.");

    for (auto const &depot : depots_)
    {
        if (depot.demand != 0)
            throw std::invalid_argument("Depot demand must be 0.");

        if (depot.serviceDuration != 0)
            throw std::invalid_argument("Depot service duration must be 0.");

        if (depot.releaseTime != 0)
            throw std::invalid_argument("Depot release time must be 0.");
    }

    for (auto const &client : clients_)
    {
        centroid_.first += static_cast<double>(client.x) / numClients();
        centroid_.second += static_cast<double>(client.y) / numClients();
    }
}
