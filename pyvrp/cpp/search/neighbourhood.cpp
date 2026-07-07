#include "neighbourhood.h"

#include "Matrix.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <set>
#include <stdexcept>
#include <tuple>

using pyvrp::Activity;
using pyvrp::Matrix;
using pyvrp::ProblemData;
using pyvrp::search::NeighbourhoodParams;

namespace
{
/**
 * Computes proximity for neighbourhood. Proximity is based on [1]_, but
 * generalised to additional VRP variants.
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
    // Rows (from) are #clients + #pickups. Columns (to) are #clients
    // + #pickups + #deliveries.
    Matrix<double> prox(data.numClients() + data.numShipments(),
                        data.numClients() + 2 * data.numShipments(),
                        std::numeric_limits<double>::max());

    std::set<std::tuple<pyvrp::Cost, pyvrp::Cost, size_t>> seen = {};
    for (auto const &vehType : data.vehicleTypes())
    {
        auto const key = std::make_tuple(vehType.unitDistanceCost,
                                         vehType.unitDurationCost,
                                         vehType.profile);

        if (seen.contains(key))  // then proximity has already been updated
            continue;            // based on this cost profile

        seen.insert(key);
        auto const &dists = data.distanceMatrix(vehType.profile);
        auto const &durs = data.durationMatrix(vehType.profile);

        auto const cost = [&](auto const &from, auto const &to)
        {
            auto const frmServ = static_cast<double>(from.serviceDuration);
            auto const frmEarly = static_cast<double>(from.twEarly);
            auto const frmLate = static_cast<double>(from.twLate);

            auto const toEarly = static_cast<double>(to.twEarly);
            auto const toLate = static_cast<double>(to.twLate);

            auto const dur = durs(from.location, to.location);
            auto const edgeDur = static_cast<double>(dur);

            if (frmEarly + frmServ + edgeDur > toLate)      // then this edge
                return std::numeric_limits<double>::max();  // is not feasible

            auto const dist = dists(from.location, to.location);
            auto const distance = static_cast<double>(dist);

            auto const minWait = toEarly - edgeDur - frmServ - frmLate;
            auto const duration = edgeDur + std::max(minWait, 0.0);

            return static_cast<double>(vehType.unitDistanceCost) * distance
                   + static_cast<double>(vehType.unitDurationCost) * duration
                   + params.weightWaitTime * std::max(minWait, 0.0);
        };

        // From clients.
        for (size_t frm = 0; frm != data.numClients(); ++frm)
        {
            auto const &frmData = data.client(frm);

            // To clients.
            for (size_t client = 0; client != data.numClients(); ++client)
            {
                auto const idx = client;
                auto const &to = data.client(client);
                prox(frm, idx) = std::min(cost(frmData, to), prox(frm, idx));
            }

            // To shipment pickups.
            for (size_t pick = 0; pick != data.numShipments(); ++pick)
            {
                auto const idx = data.numClients() + pick;
                auto const &to = data.shipment(pick).pickup;
                prox(frm, idx) = std::min(cost(frmData, to), prox(frm, idx));
            }

            // To shipment deliveries.
            for (size_t del = 0; del != data.numShipments(); ++del)
            {
                auto const idx = data.numClients() + data.numShipments() + del;
                auto const &to = data.shipment(del).delivery;
                prox(frm, idx) = std::min(cost(frmData, to), prox(frm, idx));
            }
        }

        // From shipment pickups.
        for (size_t frm = data.numClients(); frm != prox.numRows(); ++frm)
        {
            auto const &frmData = data.shipment(frm).pickup;

            // To clients.
            for (size_t client = 0; client != data.numClients(); ++client)
            {
                auto const idx = client;
                auto const &to = data.client(client);
                prox(frm, idx) = std::min(cost(frmData, to), prox(frm, idx));
            }

            // To shipment pickups.
            for (size_t pick = 0; pick != data.numShipments(); ++pick)
            {
                auto const idx = data.numClients() + pick;
                auto const &to = data.shipment(pick).pickup;
                prox(frm, idx) = std::min(cost(frmData, to), prox(frm, idx));
            }

            // To shipment deliveries.
            for (size_t del = 0; del != data.numShipments(); ++del)
            {
                auto const idx = data.numClients() + data.numShipments() + del;
                auto const &to = data.shipment(del).delivery;
                prox(frm, idx) = std::min(cost(frmData, to), prox(frm, idx));
            }
        }
    }

    return prox;
}
}  // namespace

NeighbourhoodParams::NeighbourhoodParams(double weightWaitTime,
                                         size_t numNeighbours,
                                         bool symmetricProximity)
    : weightWaitTime(weightWaitTime),
      numNeighbours(numNeighbours),
      symmetricProximity(symmetricProximity)
{
    if (numNeighbours == 0)
        throw std::invalid_argument("num_neighbours == 0 not understood.");
}

std::unordered_map<Activity, std::vector<Activity>>
pyvrp::search::computeNeighbours(ProblemData const &data,
                                 NeighbourhoodParams const &params)
{
    auto prox = computeProximity(data, params);

    if (params.symmetricProximity)  // then we symmetrise the proximity matrix
        for (size_t frm = 0; frm != prox.numRows(); ++frm)
            for (size_t to = frm; to != prox.numRows(); ++to)
                prox(frm, to) = prox(to, frm)
                    = std::min(prox(frm, to), prox(to, frm));

    for (auto const &group : data.groups())
        for (auto const frmClient : group)
            for (auto const toClient : group)
                // Group members should not neighbour each other, as only one
                // of them can be in the solution at a time. We use max float,
                // not infty: we want to avoid same group neighbours, but it is
                // not too problematic if we need to have them.
                prox(frmClient, toClient) = std::numeric_limits<double>::max();

    for (size_t idx = 0; idx != prox.numRows(); ++idx)  // excl. self
        prox(idx, idx) = std::numeric_limits<double>::infinity();

    for (size_t idx = data.numClients(); idx != prox.numRows(); ++idx)
        prox(idx, idx + data.numShipments())  // excl. own delivery
            = std::numeric_limits<double>::infinity();

    // Adjust the neigbhourhood size to the minimum of the number of other
    // clients and shipments, and the default neighbourhood size. We need to
    // make sure we do not wrap-around in case the there are no clients or
    // shipments. Since we have both pickup and delivery nodes for shipments,
    // we count those double.
    auto const numClients = std::max<size_t>(data.numClients(), 1) - 1;
    auto const numShipments = std::max<size_t>(2 * data.numShipments(), 2) - 2;
    auto const maxNeighbours = std::max(numClients, numShipments);
    auto const numNeighbours = std::min(params.numNeighbours, maxNeighbours);

    std::unordered_map<Activity, std::vector<Activity>> neighbours;

    std::vector<size_t> indices(prox.numCols());
    for (size_t idx = 0; idx != data.numClients(); ++idx)
    {
        Activity const activity = {Activity::ActivityType::CLIENT, idx};

        auto const comp = [&](auto const a, auto const b)
        { return prox(idx, a) < prox(idx, b); };

        // Reset the vector and then re-sort for this client.
        std::iota(indices.begin(), indices.end(), 0);
        std::stable_sort(indices.begin(), indices.end(), comp);

        // Neighbourhood of client is set to the first numNeighbours indices.
        neighbours[activity] = {};
        for (size_t neighbour = 0; neighbour != numNeighbours; ++neighbour)
        {
            auto &neighbourhood = neighbours[activity];

            if (indices[neighbour] < data.numClients())
                neighbourhood.emplace_back(Activity::ActivityType::CLIENT,
                                           indices[neighbour]);
            else if (indices[neighbour]
                     < data.numClients() + data.numShipments())
                neighbourhood.emplace_back(Activity::ActivityType::PICKUP,
                                           indices[neighbour]
                                               - data.numClients());
            else
                neighbourhood.emplace_back(Activity::ActivityType::DELIVERY,
                                           indices[neighbour]
                                               - data.numClients()
                                               - data.numShipments());
        }
    }

    for (size_t idx = 0; idx != data.numShipments(); ++idx)
    {
        Activity const activity = {Activity::ActivityType::PICKUP, idx};

        auto const comp = [&](auto const a, auto const b) {
            return prox(data.numClients() + idx, a)
                   < prox(data.numClients() + idx, b);
        };

        // Reset the vector and then re-sort for this pickup.
        std::iota(indices.begin(), indices.end(), 0);
        std::stable_sort(indices.begin(), indices.end(), comp);

        // Neighbourhood of pickup is set to the first numNeighbours indices.
        neighbours[activity] = {};
        for (size_t neighbour = 0; neighbour != numNeighbours; ++neighbour)
        {
            auto &neighbourhood = neighbours[activity];

            if (indices[neighbour] < data.numClients())
                neighbourhood.emplace_back(Activity::ActivityType::CLIENT,
                                           indices[neighbour]);
            else if (indices[neighbour]
                     < data.numClients() + data.numShipments())
                neighbourhood.emplace_back(Activity::ActivityType::PICKUP,
                                           indices[neighbour]
                                               - data.numClients());
            else
                neighbourhood.emplace_back(Activity::ActivityType::DELIVERY,
                                           indices[neighbour]
                                               - data.numClients()
                                               - data.numShipments());
        }
    }

    return neighbours;
}
