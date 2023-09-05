#include "ProblemData.h"

#include <numeric>

using pyvrp::Distance;
using pyvrp::Duration;
using pyvrp::Matrix;
using pyvrp::ProblemData;

ProblemData::Client::Client(Coordinate x,
                            Coordinate y,
                            Load demand,
                            Duration serviceDuration,
                            Duration twEarly,
                            Duration twLate,
                            Duration releaseTime,
                            Cost prize,
                            bool required)
    : x(x),
      y(y),
      demand(demand),
      serviceDuration(serviceDuration),
      twEarly(twEarly),
      twLate(twLate),
      releaseTime(releaseTime),
      prize(prize),
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

ProblemData::VehicleType::VehicleType(Load capacity,
                                      size_t numAvailable,
                                      Cost fixedCost,
                                      std::optional<Duration> twEarly,
                                      std::optional<Duration> twLate)
    : capacity(capacity),
      numAvailable(numAvailable),
      fixedCost(fixedCost),
      twEarly(twEarly),
      twLate(twLate)
{
    if (capacity < 0)
        throw std::invalid_argument("capacity must be >= 0.");

    if (numAvailable == 0)
        throw std::invalid_argument("num_available must be > 0.");

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
}

std::pair<double, double> const &ProblemData::centroid() const
{
    return centroid_;
}

size_t ProblemData::numClients() const { return numClients_; }

size_t ProblemData::numVehicleTypes() const { return numVehicleTypes_; }

size_t ProblemData::numVehicles() const { return numVehicles_; }

ProblemData::ProblemData(std::vector<Client> const &clients,
                         std::vector<VehicleType> const &vehicleTypes,
                         Matrix<Distance> distMat,
                         Matrix<Duration> durMat)
    : centroid_({0, 0}),
      dist_(std::move(distMat)),
      dur_(std::move(durMat)),
      clients_(clients),
      vehicleTypes_(vehicleTypes),
      numClients_(std::max<size_t>(clients.size(), 1) - 1),
      numVehicleTypes_(vehicleTypes.size()),
      numVehicles_(std::accumulate(vehicleTypes.begin(),
                                   vehicleTypes.end(),
                                   0,
                                   [](auto sum, VehicleType const &type) {
                                       return sum + type.numAvailable;
                                   }))
{
    if (dist_.numRows() != clients.size() || dist_.numCols() != clients.size())
        throw std::invalid_argument("Distance matrix shape does not match the "
                                    "number of clients.");

    if (dur_.numRows() != clients.size() || dur_.numCols() != clients.size())
        throw std::invalid_argument("Duration matrix shape does not match the "
                                    "number of clients.");

    if (clients.size() == 0)  // at least one client (the depot) is required
        throw std::invalid_argument("Clients must not be empty.");

    auto const &depot = clients[0];

    if (depot.demand != 0)
        throw std::invalid_argument("Depot demand must be 0.");

    if (depot.serviceDuration != 0)
        throw std::invalid_argument("Depot service duration must be 0.");

    if (depot.releaseTime != 0)
        throw std::invalid_argument("Depot release time must be 0.");

    for (size_t idx = 1; idx <= numClients(); ++idx)
    {
        centroid_.first += static_cast<double>(clients[idx].x) / numClients();
        centroid_.second += static_cast<double>(clients[idx].y) / numClients();
    }
}
