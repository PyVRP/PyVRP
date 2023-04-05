#include "ProblemData.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

ProblemData::Client const &ProblemData::depot() const { return client(0); }

Matrix<int> const &ProblemData::distanceMatrix() const { return dist_; }

size_t ProblemData::numClients() const { return numClients_; }

size_t ProblemData::numVehicles() const { return numVehicles_; }

size_t ProblemData::vehicleCapacity() const { return vehicleCapacity_; }

ProblemData::ProblemData(std::vector<Client> const &clients,
                         size_t numVehicles,
                         size_t vehicleCap,
                         std::vector<std::vector<int>> const &distMat)
    : dist_(distMat),
      clients_(clients),
      numClients_(clients.size() - 1),
      numVehicles_(numVehicles),
      vehicleCapacity_(vehicleCap)
{
}
