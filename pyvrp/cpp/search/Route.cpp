#define _USE_MATH_DEFINES  // needed to get M_PI etc. on Windows builds
// TODO use std::numbers::pi instead of M_PI when C++20 is supported by CIBW

#include "Route.h"

#include <cmath>
#include <ostream>

using TWS = TimeWindowSegment;

Route::Route(ProblemData const &data) : data(data) {}

void Route::setupNodes()
{
    nodes.clear();
    auto *node = depot;

    do
    {
        node = n(node);
        nodes.push_back(node);
    } while (!node->isDepot());
}

void Route::setupSector()
{
    if (empty())  // Note: sector has no meaning for empty routes, don't use
        return;

    auto const &depotData = data.client(0);
    auto const &clientData = data.client(n(depot)->client);

    auto const diffX = static_cast<double>(clientData.x - depotData.x);
    auto const diffY = static_cast<double>(clientData.y - depotData.y);
    auto const angle = CircleSector::positive_mod(
        static_cast<int>(32768. * atan2(diffY, diffX) / M_PI));

    sector.initialize(angle);

    for (auto it = nodes.begin(); it != nodes.end() - 1; ++it)
    {
        auto const *node = *it;
        assert(!node->isDepot());

        auto const &clientData = data.client(node->client);

        auto const diffX = static_cast<double>(clientData.x - depotData.x);
        auto const diffY = static_cast<double>(clientData.y - depotData.y);
        auto const angle = CircleSector::positive_mod(
            static_cast<int>(32768. * atan2(diffY, diffX) / M_PI));

        sector.extend(angle);
    }
}

void Route::setupRouteTimeWindows()
{
    auto *node = nodes.back();

    do  // forward time window segments
    {
        auto *prev = p(node);
        prev->twAfter
            = TWS::merge(data.durationMatrix(), prev->tw, node->twAfter);
        node = prev;
    } while (!node->isDepot());
}

bool Route::overlapsWith(Route const &other, int const tolerance) const
{
    return CircleSector::overlap(sector, other.sector, tolerance);
}

void Route::update()
{
    std::cout << "###### Enter Route:update" << std::endl;
    auto const oldNodes = nodes;
    setupNodes();

    Load weight = 0;
    Load volume = 0;
    Salvage salvage = 0;
    Distance distance = 0;
    Distance reverseDistance = 0;
    bool foundChange = false;
    bool foundSalvage = false;
    bool isSalvage = false;
    bool salvageBeforeDelivery = false;
    bool isDelivery = false;
    Salvage salvageCount = 0;

    // std::cout << std::endl;
    // std::cout << "Before Loop " << "salvageBeforeDelivery: " << salvageBeforeDelivery << ", salvageCount: " << salvageCount << std::endl;
    for (size_t pos = 0; pos != nodes.size(); ++pos)
    {
        // std::cout << "Position: " << pos << std::endl;
        auto *node = nodes[pos];

        if (!foundChange && (pos >= oldNodes.size() || node != oldNodes[pos]))
        {
            foundChange = true;

            if (pos > 0)
            {
                weight = nodes[pos - 1]->cumulatedWeight;
                volume = nodes[pos - 1]->cumulatedVolume;
                salvage = nodes[pos - 1]->cumulatedSalvage;
                distance = nodes[pos - 1]->cumulatedDistance;
                reverseDistance = nodes[pos - 1]->cumulatedReversalDistance;
            }
        }

        if (!foundChange)
            continue;

        weight += data.client(node->client).demandWeight;
        volume += data.client(node->client).demandVolume;
        salvage += data.client(node->client).demandSalvage;

        if (data.client(node->client).demandSalvage)
        {
            isSalvage = true;
        }

        if (data.client(node->client).demandWeight || data.client(node->client).demandVolume)
        {
            isDelivery = true;
        }
    
        if (isDelivery && foundSalvage)
        {
            salvageBeforeDelivery = true;
        }

        salvageCount = data.client(node->client).demandSalvage ? salvageCount + Salvage(1) : salvageCount;

        if (isSalvage)
        {
            if (!foundSalvage)
                foundSalvage = true;
        }


        // if (data.client(node->client).demandSalvage == 1) {
        //     // std::cout << "demandSalvage == 1 for client: " << node->client << std::endl;
        //     if (!foundDelivery) {
        //         // std::cout << "foundDelivery State: " << foundDelivery << std::endl;
        //         // std::cout << "SalvageCount Before: " << salvageCount << std::endl;
        //         salvageCount = data.salvageCapacity() + Salvage(1);
        //         // std::cout << "SalvageCount After: " << salvageCount << std::endl;
        //         // break;
        //     }
        //     salvageCount = salvageCount + Salvage(1);
        // }
        // else if(!foundDelivery){
        //     foundDelivery = true;
        // }

        distance += data.dist(p(node)->client, node->client);

        reverseDistance += data.dist(node->client, p(node)->client);
        reverseDistance -= data.dist(p(node)->client, node->client);

        node->position = pos + 1;
        node->cumulatedWeight = weight;
        node->cumulatedVolume = volume;
        node->cumulatedSalvage = salvage;
        node->cumulatedDistance = distance;
        node->cumulatedReversalDistance = reverseDistance;

        // Debug prints
        // std::cout << "Node: " << node->client 
        //           << ", Position: " << node->position 
        //           << ", CumulatedWeight: " << node->cumulatedWeight 
        //           << ", CumulatedVolume: " << node->cumulatedVolume << std::endl;

        node->twBefore
            = TWS::merge(data.durationMatrix(), p(node)->twBefore, node->tw);

        std::cout << "Route Constructor Node: " << pos
            << " salvageCount: " << salvageCount
            << " demandVolume: " << data.client(node->client).demandVolume
            << " demandSalvage: " << data.client(node->client).demandSalvage
            << " foundSalvage: " << foundSalvage
            << " SalvageBefore: " << salvageBeforeDelivery
            << std::endl;

        // std::cout << "Inside Loop " << "salvageBeforeDelivery: " << salvageBeforeDelivery << ", salvageCount: " << salvageCount << std::endl;
    }
    // std::cout << "After Loop" << std::endl;
    setupSector();
    setupRouteTimeWindows();

    weight_ = nodes.back()->cumulatedWeight;
    volume_ = nodes.back()->cumulatedVolume;
    salvage_ = nodes.back()->cumulatedSalvage;

    isWeightFeasible_ = static_cast<size_t>(weight_) <= data.weightCapacity();
    isVolumeFeasible_ = static_cast<size_t>(volume_) <= data.volumeCapacity();

    timeWarp_ = nodes.back()->twBefore.totalTimeWarp();
    isTimeWarpFeasible_ = timeWarp_ == 0;

    // isSalvageCapacityFeasible_ = salvageCount <= data.salvageCapacity();
    isSalvageCapacityFeasible_ = static_cast<size_t>(salvage_) <= data.salvageCapacity();
    isSalvageSequenceFeasible_ = !salvageBeforeDelivery;

    // Debug prints
    std::cout << "SalvageCount: " << salvageCount 
              << ", isSalvageCapacityFeasible: " << isSalvageCapacityFeasible_ << ", isSalvageSequenceFeasible: " << isSalvageSequenceFeasible_ << std::endl;
    std::cout << "Final weight: " << weight_
              << ", isWeightFeasible: " << isWeightFeasible_
              << ", Final volume: " << volume_
              << ", isVolumeFeasible: " << isVolumeFeasible_ << std::endl;
    std::cout << "###### Exit Route:Update" << std::endl << std::endl;
}

Route* Route::clone() const
{
    // Create a new Route object.
    Route* clonedRoute = new Route(this->data);

    // Copy simple data members.
    clonedRoute->weight_ = this->weight_;
    clonedRoute->volume_ = this->volume_;
    clonedRoute->salvage_ = this->salvage_;
    clonedRoute->isWeightFeasible_ = this->isWeightFeasible_;
    clonedRoute->isVolumeFeasible_ = this->isVolumeFeasible_;
    clonedRoute->isSalvageCapacityFeasible_ = this->isSalvageCapacityFeasible_;
    clonedRoute->isSalvageSequenceFeasible_ = this->isSalvageSequenceFeasible_;
    clonedRoute->timeWarp_ = this->timeWarp_;
    clonedRoute->isTimeWarpFeasible_ = this->isTimeWarpFeasible_;
    clonedRoute->idx = this->idx;
    clonedRoute->depot = this->depot;

    // Copy nodes.
    for (auto &node : this->nodes) {
        Node* clonedNode = new Node(*node);
        clonedRoute->nodes.push_back(clonedNode);
    }

    // Set up the sector, nodes, and time windows.
    clonedRoute->setupSector();
    clonedRoute->setupNodes();
    clonedRoute->setupRouteTimeWindows();

    return clonedRoute;
}

std::ostream &operator<<(std::ostream &out, Route const &route)
{
    out << "Route #" << route.idx + 1 << ":";  // route number
    for (auto *node = n(route.depot); !node->isDepot(); node = n(node))
        out << ' ' << node->client;  // client index
    out << '\n';

    return out;
}
