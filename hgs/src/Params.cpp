#include "Params.h"

#include "Matrix.h"
#include "XorShift128.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <string>
#include <vector>

Params::Params(Config const &config, std::string const &instPath)
    : config(config)
{
    // Initialize some parameter values
    std::string content, content2, content3;
    int serviceTimeData = 0;
    int node;
    bool hasServiceTimeSection = false;
    nbClients = 0;
    int totalDemand = 0;
    int maxDemand = 0;
    vehicleCapacity = INT_MAX;

    // Read INPUT dataset
    std::ifstream inputFile(instPath);

    if (!inputFile)
        throw std::invalid_argument("Impossible to open file: " + instPath);

    // Read the instance name from the first line and remove it
    getline(inputFile, content);

    // Read the next lines
    getline(inputFile,
            content);             // "Empty line" or "NAME : {instance_name}"
    getline(inputFile, content);  // VEHICLE or "COMMENT: {}"

    // Check if the next line has "VEHICLE"
    if (content.substr(0, 7) == "VEHICLE")
    {
        // Get the number of vehicles and the capacity of the vehicles
        getline(inputFile, content);  // NUMBER    CAPACITY
        inputFile >> nbVehicles >> vehicleCapacity;

        // Skip the next four lines
        getline(inputFile, content);
        getline(inputFile, content);
        getline(inputFile, content);
        getline(inputFile, content);

        // Create a vector where all information on the clients can be
        // stored and loop over all information in the file
        clients = std::vector<Client>(1001);
        nbClients = 0;
        while (inputFile >> node)
        {
            // Store all the information of the next client
            inputFile >> clients[nbClients].x >> clients[nbClients].y
                >> clients[nbClients].demand >> clients[nbClients].twEarly
                >> clients[nbClients].twLate >> clients[nbClients].servDur;

            // Scale coordinates by factor 10, later the distances will be
            // rounded so we optimize with 1 decimal distances
            clients[nbClients].x *= 10;
            clients[nbClients].y *= 10;
            clients[nbClients].twEarly *= 10;
            clients[nbClients].twLate *= 10;
            clients[nbClients].servDur *= 10;

            // Keep track of the max demand, the total demand, and the
            // number of clients
            if (clients[nbClients].demand > maxDemand)
            {
                maxDemand = clients[nbClients].demand;
            }
            totalDemand += clients[nbClients].demand;
            nbClients++;
        }

        // Reduce the size of the vector of clients if possible
        clients.resize(nbClients);

        // Don't count depot as client
        nbClients--;

        // Check if the required service and the start of the time window of
        // the depot are both zero
        if (clients[0].twEarly != 0)
        {
            throw std::runtime_error("Depot time window should start at 0");
        }
        if (clients[0].servDur != 0)
        {
            throw std::runtime_error("Depot service duration should be 0");
        }
    }
    else
    {
        // CVRP or VRPTW according to VRPLib format
        for (inputFile >> content; content != "EOF"; inputFile >> content)
        {
            // Read the dimension of the problem (the number of clients)
            if (content == "DIMENSION")
            {
                // Need to substract the depot from the number of nodes
                inputFile >> content2 >> nbClients;
                nbClients--;
            }
            // Read the type of edge weights
            else if (content == "EDGE_WEIGHT_TYPE")
            {
                inputFile >> content2 >> content3;
            }
            else if (content == "EDGE_WEIGHT_FORMAT")
            {
                inputFile >> content2 >> content3;
                if (content3 != "FULL_MATRIX")
                {
                    throw std::runtime_error(
                        "EDGE_WEIGHT_FORMAT only supports FULL_MATRIX");
                }
            }
            else if (content == "CAPACITY")
            {
                inputFile >> content2 >> vehicleCapacity;
            }
            else if (content == "VEHICLES" || content == "SALESMAN")
            {
                // EURO/NeurIPS allows unlimited vehicles, so this is a no-op.
                std::string content2_;
                int nbVehicles_;

                inputFile >> content2_ >> nbVehicles_;
            }
            // Read the data on the service time (used when the service time
            // is constant for all clients)
            else if (content == "SERVICE_TIME")
            {
                inputFile >> content2 >> serviceTimeData;
            }
            // Read the edge weights of an explicit distance matrix
            else if (content == "EDGE_WEIGHT_SECTION")
            {
                dist_ = Matrix<int>(nbClients + 1);
                for (int i = 0; i <= nbClients; i++)
                {
                    for (int j = 0; j <= nbClients; j++)
                    {
                        // Keep track of the largest distance between two
                        // clients (or the depot)
                        inputFile >> dist(i, j);
                    }
                }
            }
            else if (content == "NODE_COORD_SECTION")
            {
                // Reading client coordinates
                clients = std::vector<Client>(nbClients + 1);
                for (int i = 0; i <= nbClients; i++)
                {
                    int localNode;
                    inputFile >> localNode >> clients[i].x >> clients[i].y;

                    // Check if the clients are in order
                    if (localNode != i + 1)
                    {
                        throw std::runtime_error("Coordinates are not in "
                                                 "order of clients");
                    }
                }
            }
            // Read the demand of each client (including the depot, which
            // should have demand 0)
            else if (content == "DEMAND_SECTION")
            {
                for (int i = 0; i <= nbClients; i++)
                {
                    int clientNr = 0;
                    inputFile >> clientNr >> clients[i].demand;

                    // Check if the clients are in order
                    if (clientNr != i + 1)
                    {
                        throw std::runtime_error("Clients are not in order"
                                                 " in the list of demands");
                    }

                    // Keep track of the max and total demand
                    if (clients[i].demand > maxDemand)
                    {
                        maxDemand = clients[i].demand;
                    }
                    totalDemand += clients[i].demand;
                }
                // Check if the depot has demand 0
                if (clients[0].demand != 0)
                {
                    throw std::runtime_error(
                        "Depot demand is not zero, but is instead: "
                        + std::to_string(clients[0].servDur));
                }
            }
            else if (content == "DEPOT_SECTION")
            {
                inputFile >> content2 >> content3;
                if (content2 != "1")
                {
                    throw std::runtime_error("Expected depot index 1 "
                                             "instead of "
                                             + content2);
                }
            }
            else if (content == "SERVICE_TIME_SECTION")
            {
                for (int i = 0; i <= nbClients; i++)
                {
                    int clientNr = 0;
                    inputFile >> clientNr >> clients[i].servDur;

                    // Check if the clients are in order
                    if (clientNr != i + 1)
                    {
                        throw std::runtime_error("Service times are not "
                                                 "in client order");
                    }
                }
                // Check if the service duration of the depot is 0
                if (clients[0].servDur != 0)
                {
                    throw std::runtime_error(
                        "Service duration for depot should be 0");
                }
                hasServiceTimeSection = true;
            }
            else if (content == "RELEASE_TIME_SECTION")
            {
                for (int i = 0; i <= nbClients; i++)
                {
                    int clientNr = 0;
                    inputFile >> clientNr >> clients[i].releaseTime;

                    // Check if the clients are in order
                    if (clientNr != i + 1)
                    {
                        throw std::runtime_error("Release times are not in"
                                                 " client order");
                    }
                }
                // Check if the service duration of the depot is 0
                if (clients[0].releaseTime != 0)
                {
                    throw std::runtime_error(
                        "Release time for depot should be 0");
                }
            }
            // Read the time windows of all the clients (the depot should
            // have a time window from 0 to max)
            else if (content == "TIME_WINDOW_SECTION")
            {
                for (int i = 0; i <= nbClients; i++)
                {
                    int clientNr = 0;
                    inputFile >> clientNr >> clients[i].twEarly
                        >> clients[i].twLate;

                    // Check if the clients are in order
                    if (clientNr != i + 1)
                    {
                        throw std::runtime_error("Time windows are not in "
                                                 "client order");
                    }
                }

                // Check the start of the time window of the depot
                if (clients[0].twEarly != 0)
                {
                    throw std::runtime_error(
                        "Time window for depot should start at 0");
                }
            }
            else
            {
                throw std::runtime_error("Unexpected data in input file: "
                                         + content);
            }
        }

        if (!hasServiceTimeSection)
        {
            for (int i = 0; i <= nbClients; i++)
            {
                clients[i].servDur = (i == 0) ? 0 : serviceTimeData;
            }
        }

        if (nbClients <= 0)
        {
            throw std::runtime_error("Number of nodes is undefined");
        }
        if (vehicleCapacity == INT_MAX)
            throw std::runtime_error("Vehicle capacity is undefined");
    }

    nbVehicles = config.nbVeh >= nbClients ? nbClients : config.nbVeh;
    maxDist_ = dist_.max();

    // Calculate, for all vertices, the correlation for the nbGranular closest
    // vertices
    calculateNeighbours();

    // Safeguards to avoid possible numerical instability in case of instances
    // containing arbitrarily small or large numerical values
    if (maxDist_ < 0.1 || maxDist_ > 100000)
    {
        throw std::runtime_error(
            "The distances are of very small or large scale. This could impact "
            "numerical stability. Please rescale the dataset and run again.");
    }
    if (maxDemand < 0.1 || maxDemand > 100000)
    {
        throw std::runtime_error(
            "The demand quantities are of very small or large scale. This "
            "could impact numerical stability. Please rescale the dataset and "
            "run again.");
    }
    if (nbVehicles < std::ceil(totalDemand / vehicleCapacity))
    {
        throw std::runtime_error(
            "Fleet size is insufficient to service the considered clients.");
    }

    // A reasonable scale for the initial values of the penalties
    penaltyCapacity = std::max(1, std::min(1000, maxDist_ / maxDemand));

    // Initial parameter values of this parameter is not argued
    penaltyTimeWarp = static_cast<int>(config.initialTimeWarpPenalty);
}

Params::Params(Config const &config,
               std::vector<std::pair<int, int>> const &coords,
               std::vector<int> const &demands,
               int vehicleCap,
               std::vector<std::pair<int, int>> const &timeWindows,
               std::vector<int> const &servDurs,
               std::vector<std::vector<int>> const &distMat,
               std::vector<int> const &releases)
    : config(config),
      nbClients(static_cast<int>(coords.size()) - 1),
      nbVehicles(std::max(std::min(config.nbVeh, nbClients), 1)),
      vehicleCapacity(vehicleCap)
{
    dist_ = Matrix<int>(distMat.size());

    for (size_t i = 0; i != distMat.size(); ++i)
        for (size_t j = 0; j != distMat[i].size(); ++j)
            dist(i, j) = distMat[i][j];

    maxDist_ = dist_.max();

    // A reasonable scale for the initial values of the penalties
    int const maxDemand = *std::max_element(demands.begin(), demands.end());
    int const initCapPenalty = maxDist_ / std::max(maxDemand, 1);
    penaltyCapacity = std::max(std::min(1000, initCapPenalty), 1);

    // Initial parameter values of this parameter is not argued
    penaltyTimeWarp = static_cast<int>(config.initialTimeWarpPenalty);

    clients = std::vector<Client>(nbClients + 1);

    for (size_t idx = 0; idx <= static_cast<size_t>(nbClients); ++idx)
        clients[idx] = {coords[idx].first,
                        coords[idx].second,
                        servDurs[idx],
                        demands[idx],
                        timeWindows[idx].first,
                        timeWindows[idx].second,
                        releases[idx]};

    calculateNeighbours();
}

void Params::calculateNeighbours()
{
    auto proximities
        = std::vector<std::vector<std::pair<int, int>>>(nbClients + 1);

    for (int i = 1; i <= nbClients; i++)  // exclude depot
    {
        auto &proximity = proximities[i];

        for (int j = 1; j <= nbClients; j++)  // exclude depot
        {
            if (i == j)  // exclude the current client
                continue;

            // Compute proximity using Eq. 4 in Vidal 2012. The proximity is
            // computed by the distance, min. wait time and min. time warp
            // going from either i -> j or j -> i, whichever is the least.
            int const maxRelease
                = std::max(clients[i].releaseTime, clients[j].releaseTime);

            // Proximity from j to i
            int const waitTime1 = clients[i].twEarly - dist(j, i)
                                  - clients[j].servDur - clients[j].twLate;
            int const earliestArrival1
                = std::max(maxRelease + dist(0, j), clients[j].twEarly);
            int const timeWarp1 = earliestArrival1 + clients[j].servDur
                                  + dist(j, i) - clients[i].twLate;
            int const prox1 = dist(j, i)
                              + config.weightWaitTime * std::max(0, waitTime1)
                              + config.weightTimeWarp * std::max(0, timeWarp1);

            // Proximity from i to j
            int const waitTime2 = clients[j].twEarly - dist(i, j)
                                  - clients[i].servDur - clients[i].twLate;
            int const earliestArrival2
                = std::max(maxRelease + dist(0, i), clients[i].twEarly);
            int const timeWarp2 = earliestArrival2 + clients[i].servDur
                                  + dist(i, j) - clients[j].twLate;
            int const prox2 = dist(i, j)
                              + config.weightWaitTime * std::max(0, waitTime2)
                              + config.weightTimeWarp * std::max(0, timeWarp2);

            proximity.emplace_back(std::min(prox1, prox2), j);
        }

        std::sort(proximity.begin(), proximity.end());
    }

    neighbours = std::vector<std::vector<int>>(nbClients + 1);

    // First create a set of correlated vertices for each vertex (where the
    // depot is not taken into account)
    std::vector<std::set<int>> set(nbClients + 1);
    size_t const granularity
        = std::min(config.nbGranular, static_cast<size_t>(nbClients) - 1);

    for (int i = 1; i <= nbClients; i++)  // again exclude depot
    {
        auto const &orderProximity = proximities[i];

        for (size_t j = 0; j != granularity; ++j)
            set[i].insert(orderProximity[j].second);
    }

    for (int i = 1; i <= nbClients; i++)
        for (int x : set[i])
            neighbours[i].push_back(x);
}
