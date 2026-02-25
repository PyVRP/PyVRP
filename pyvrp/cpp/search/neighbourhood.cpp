#include "neighbourhood.h"

#include "Matrix.h"

#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>
#include <tuple>

using pyvrp::Matrix;
using pyvrp::ProblemData;
using pyvrp::search::NeighbourhoodParams;

namespace
{
/**
 * Computes proximity for neighbourhood. Proximity is based on [1]_, with
 * modification for additional VRP variants.
 *
 * References
 * ----------
 * .. [1] Vidal, T., Crainic, T. G., Gendreau, M., and Prins, C. (2013). A
 *        hybrid genetic algorithm with adaptive diversity management for a
 *        large class of vehicle routing problems with time-windows.
 *        *Computers & Operations Research*, 40(1), 475 - 489.
 */
Matrix<double> computeProximity(ProblemData const &data,
                                NeighbourhoodParams const &params)
{
    Matrix<double> prox(data.numLocations(),
                        data.numLocations(),
                        std::numeric_limits<double>::max());

    std::set<std::tuple<pyvrp::Cost, pyvrp::Cost, size_t>> seen = {};

    for (auto const &vehType : data.vehicleTypes())
    {
        auto const key = std::make_tuple(vehType.unitDistanceCost,
                                         vehType.unitDurationCost,
                                         vehType.profile);

        if (seen.contains(key))
            continue;

        seen.insert(key);
        auto const &distMat = data.distanceMatrix(vehType.profile);
        auto const &durMat = data.durationMatrix(vehType.profile);

        for (size_t i = 0; i != data.numLocations(); ++i)
            for (size_t j = 0; j != data.numLocations(); ++j)
            {
                auto const dist = static_cast<pyvrp::Cost>(distMat(i, j));
                auto const dur = static_cast<pyvrp::Cost>(durMat(i, j));
                auto const value
                    = static_cast<double>(vehType.unitDistanceCost * dist)
                      + static_cast<double>(vehType.unitDurationCost * dur);

                prox(i, j) = std::min(value, prox(i, j));
            }
    }

    for (size_t i = data.numDepots(); i != data.numLocations(); ++i)
    {
        ProblemData::Client const &iData = data.location(i);
        auto const iService = static_cast<double>(iData.serviceDuration);
        auto const iEarly = static_cast<double>(iData.twEarly);
        auto const iLate = static_cast<double>(iData.twLate);

        for (size_t j = data.numDepots(); j != data.numLocations(); ++j)
        {
            auto minDuration = std::numeric_limits<pyvrp::Duration>::max();
            for (auto const &mat : data.durationMatrices())
                if (mat(i, j) < minDuration)
                    minDuration = mat(i, j);

            ProblemData::Client const &jData = data.location(j);
            auto const jEarly = static_cast<double>(jData.twEarly);
            auto const jLate = static_cast<double>(jData.twLate);

            auto const minDur = static_cast<double>(minDuration);
            auto const minWait = jEarly - minDur - iService - iLate;
            auto const minTw = iEarly + iService + minDur - jLate;

            prox(i, j) -= static_cast<double>(jData.prize);
            prox(i, j) += params.weightWaitTime * std::max(minWait, 0.0);
            prox(i, j) += params.weightTimeWarp * std::max(minTw, 0.0);
        }
    }

    return prox;
}
}  // namespace

NeighbourhoodParams::NeighbourhoodParams(double weightWaitTime,
                                         double weightTimeWarp,
                                         size_t numNeighbours,
                                         bool symmetricProximity)
    : weightWaitTime(weightWaitTime),
      weightTimeWarp(weightTimeWarp),
      numNeighbours(numNeighbours),
      symmetricProximity(symmetricProximity)
{
    if (numNeighbours == 0)
        throw std::invalid_argument("num_neighbours == 0 not understood.");
}

std::vector<std::vector<size_t>>
pyvrp::search::computeNeighbours(ProblemData const &data,
                                 NeighbourhoodParams const &params)
{
    auto prox = computeProximity(data, params);

    if (params.symmetricProximity)
        for (size_t i = 0; i != data.numLocations(); ++i)
            for (size_t j = i; j != data.numLocations(); ++j)
                prox(i, j) = prox(j, i) = std::min(prox(i, j), prox(j, i));

    for (auto const &group : data.groups())
        for (auto const iClient : group)
            for (auto const jClient : group)
                prox(iClient, jClient) = std::numeric_limits<double>::max();

    for (size_t depot = 0; depot != data.numLocations(); ++depot)
        prox(depot, depot) = std::numeric_limits<double>::infinity();

    size_t const numClients = std::max(data.numClients(), 1UL);
    size_t const k = std::min(params.numNeighbours, numClients - 1);

    std::vector<std::vector<size_t>> result(data.numLocations());
    for (size_t client = data.numDepots(); client != data.numLocations();
         ++client)
    {
        std::vector<size_t> indices(data.numClients());
        std::iota(indices.begin(), indices.end(), data.numDepots());

        auto const comp = [&](auto const a, auto const b)
        { return prox(client, a) < prox(client, b); };

        std::stable_sort(indices.begin(), indices.end(), comp);

        result[client] = {indices.begin(), indices.begin() + k};
    }

    return result;
}
