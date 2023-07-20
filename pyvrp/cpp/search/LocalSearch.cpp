#include "LocalSearch.h"
#include "Measure.h"
#include "TimeWindowSegment.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <set>
#include <stdexcept>
#include <vector>

using TWS = TimeWindowSegment;

Solution LocalSearch::search(Solution &solution,
                             CostEvaluator const &costEvaluator)
{
    loadSolution(solution);

    if (nodeOps.empty())
        throw std::runtime_error("No known node operators.");

    // Caches the last time nodes were tested for modification (uses numMoves to
    // track this). The lastModified field, in contrast, track when a route was
    // last *actually* modified.
    std::vector<int> lastTestedNodes(data.numClients() + 1, -1);
    lastModified = std::vector<int>(data.numVehicles(), 0);

    searchCompleted = false;
    numMoves = 0;

    for (int step = 0; !searchCompleted; ++step)
    {
        searchCompleted = true;

        // Node operators are evaluated at neighbouring (U, V) pairs.
        for (auto const uClient : orderNodes)
        {
            auto *U = &clients[uClient];

            auto const lastTestedNode = lastTestedNodes[uClient];
            lastTestedNodes[uClient] = numMoves;

            if (U->route && !data.client(uClient).required)  // test removing U
                maybeRemove(U, costEvaluator);

            // Shuffling the neighbours in this loop should not matter much as
            // we are already randomizing the nodes U.
            for (auto const vClient : neighbours[uClient])
            {
                auto *V = &clients[vClient];

                if (!U->route && V->route)             // U might be inserted
                    maybeInsert(U, V, costEvaluator);  // into V's route

                if (!U->route || !V->route)  // we already tested inserting U,
                    continue;                // so we can skip this move

                if (lastModified[U->route->idx] > lastTestedNode
                    || lastModified[V->route->idx] > lastTestedNode)
                {
                    if (applyNodeOps(U, V, costEvaluator))
                        continue;

                    if (p(V)->isDepot() && applyNodeOps(U, p(V), costEvaluator))
                        continue;
                }
            }

            if (step > 0)  // empty moves are not tested initially to avoid
            {              // using too many routes.
                auto pred = [](auto const &route) { return route.empty(); };
                auto empty = std::find_if(routes.begin(), routes.end(), pred);

                if (empty == routes.end())
                    continue;

                if (U->route)  // try inserting U into the empty route.
                    applyNodeOps(U, empty->depot, costEvaluator);
                else  // U is not in the solution, so again try inserting.
                    maybeInsert(U, empty->depot, costEvaluator);
            }
        }
    }

    return exportSolution();
}

Solution LocalSearch::intensify(Solution &solution,
                                CostEvaluator const &costEvaluator,
                                int overlapToleranceDegrees)
{
    loadSolution(solution);

    auto const overlapTolerance = overlapToleranceDegrees * 65536;

    if (routeOps.empty())
        throw std::runtime_error("No known route operators.");

    std::vector<int> lastTestedRoutes(data.numVehicles(), -1);
    lastModified = std::vector<int>(data.numVehicles(), 0);

    searchCompleted = false;
    numMoves = 0;

    while (!searchCompleted)
    {
        searchCompleted = true;

        for (int const rU : orderRoutes)
        {
            auto &U = routes[rU];

            if (U.empty())
                continue;

            auto const lastTested = lastTestedRoutes[U.idx];
            lastTestedRoutes[U.idx] = numMoves;

            // Shuffling in this loop should not matter much as we are
            // already randomizing the routes U.
            for (int rV = 0; rV != U.idx; ++rV)
            {
                auto &V = routes[rV];

                if (V.empty() || !U.overlapsWith(V, overlapTolerance))
                    continue;

                auto const lastModifiedRoute
                    = std::max(lastModified[U.idx], lastModified[V.idx]);

                if (lastModifiedRoute > lastTested
                    && applyRouteOps(&U, &V, costEvaluator))
                    continue;
            }
        }
    }

    return exportSolution();
}

void LocalSearch::shuffle(XorShift128 &rng)
{
    std::shuffle(orderNodes.begin(), orderNodes.end(), rng);
    std::shuffle(nodeOps.begin(), nodeOps.end(), rng);

    std::shuffle(orderRoutes.begin(), orderRoutes.end(), rng);
    std::shuffle(routeOps.begin(), routeOps.end(), rng);
}

bool LocalSearch::applyNodeOps(Node *U,
                               Node *V,
                               CostEvaluator const &costEvaluator)
{
    for (auto *nodeOp : nodeOps)
        if (nodeOp->evaluate(U, V, costEvaluator) < 0)
        {
            auto *routeU = U->route;  // copy pointers because the operator can
            auto *routeV = V->route;  // modify the node's route membership

            nodeOp->apply(U, V);
            update(routeU, routeV);

            return true;
        }

    return false;
}

bool LocalSearch::applyRouteOps(Route *U,
                                Route *V,
                                CostEvaluator const &costEvaluator)
{
    for (auto *routeOp : routeOps)
        if (routeOp->evaluate(U, V, costEvaluator) < 0)
        {
            routeOp->apply(U, V);
            update(U, V);

            for (auto *op : routeOps)  // this is used by some route operators
            {                          // (particularly SWAP*) to keep caches
                op->update(U);         // in sync.
                op->update(V);
            }

            return true;
        }

    return false;
}

void LocalSearch::maybeInsert(Node *U,
                              Node *V,
                              CostEvaluator const &costEvaluator)
{
    assert(!U->route && V->route);

    Distance const deltaDist = data.dist(V->client, U->client)
                               + data.dist(U->client, n(V)->client)
                               - data.dist(V->client, n(V)->client);

    auto const &uClient = data.client(U->client);
    Cost deltaCost = static_cast<Cost>(deltaDist) - uClient.prize;

    deltaCost += costEvaluator.weightPenalty(V->route->weight() + uClient.demandWeight,
                                           data.weightCapacity());
    deltaCost += costEvaluator.volumePenalty(V->route->volume() + uClient.demandVolume,
                                           data.volumeCapacity());
    deltaCost += costEvaluator.salvagePenalty(V->route->salvage() + uClient.demandSalvage,
                                           data.salvageCapacity());

    deltaCost
        -= costEvaluator.weightPenalty(V->route->weight(), data.weightCapacity());
    deltaCost
        -= costEvaluator.volumePenalty(V->route->volume(), data.volumeCapacity());
    deltaCost
        -= costEvaluator.salvagePenalty(V->route->salvage(), data.salvageCapacity());

    // If this is true, adding U cannot decrease time warp in V's route enough
    // to offset the deltaCost.
    if (deltaCost >= costEvaluator.twPenalty(V->route->timeWarp()))
        return;

    auto const vTWS
        = TWS::merge(data.durationMatrix(), V->twBefore, U->tw, n(V)->twAfter);

    deltaCost += costEvaluator.twPenalty(vTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(V->route->timeWarp());

    if (deltaCost < 0)
    {
        U->insertAfter(V);           // U has no route, so there's nothing to
        update(V->route, V->route);  // update there.
    }
}

void LocalSearch::maybeRemove(Node *U, CostEvaluator const &costEvaluator)
{
    assert(U->route);

    Distance const deltaDist = data.dist(p(U)->client, n(U)->client)
                               - data.dist(p(U)->client, U->client)
                               - data.dist(U->client, n(U)->client);

    auto const &uClient = data.client(U->client);
    Cost deltaCost = static_cast<Cost>(deltaDist) + uClient.prize;

    deltaCost += costEvaluator.weightPenalty(U->route->weight() - uClient.demandWeight,
                                           data.weightCapacity());
    deltaCost += costEvaluator.volumePenalty(U->route->volume() - uClient.demandVolume,
                                           data.volumeCapacity());
    deltaCost += costEvaluator.salvagePenalty(U->route->salvage() - uClient.demandSalvage,
                                           data.salvageCapacity());
    deltaCost
        -= costEvaluator.weightPenalty(U->route->weight(), data.weightCapacity());
    deltaCost
        -= costEvaluator.volumePenalty(U->route->volume(), data.volumeCapacity());
    deltaCost
        -= costEvaluator.salvagePenalty(U->route->salvage(), data.salvageCapacity());

    auto uTWS
        = TWS::merge(data.durationMatrix(), p(U)->twBefore, n(U)->twAfter);

    deltaCost += costEvaluator.twPenalty(uTWS.totalTimeWarp());
    deltaCost -= costEvaluator.twPenalty(U->route->timeWarp());

    if (deltaCost < 0)
    {
        auto *route = U->route;  // after U->remove(), U->route is a nullptr
        U->remove();
        update(route, route);
    }
}

void LocalSearch::update(Route *U, Route *V)
{
    numMoves++;
    searchCompleted = false;

    U->update();
    lastModified[U->idx] = numMoves;

    if (U != V)
    {
        V->update();
        lastModified[V->idx] = numMoves;
    }
}

void LocalSearch::loadSolution(Solution const &solution)
{
    for (size_t client = 0; client <= data.numClients(); client++)
    {
        clients[client].tw = {static_cast<int>(client),  // TODO cast
                              static_cast<int>(client),  // TODO cast
                              data.client(client).serviceDuration,
                              0,
                              data.client(client).twEarly,
                              data.client(client).twLate};

        clients[client].route = nullptr;  // nullptr implies "not in solution"
    }

    auto const &solRoutes = solution.getRoutes();

    for (size_t r = 0; r != data.numVehicles(); r++)
    {
        Node *startDepot = &startDepots[r];
        Node *endDepot = &endDepots[r];

        startDepot->prev = endDepot;
        startDepot->next = endDepot;

        endDepot->prev = startDepot;
        endDepot->next = startDepot;

        startDepot->tw = clients[0].tw;
        startDepot->twBefore = clients[0].tw;

        endDepot->tw = clients[0].tw;
        endDepot->twAfter = clients[0].tw;

        Route *route = &routes[r];

        if (r < solRoutes.size())
        {
            Node *client = &clients[solRoutes[r][0]];
            client->route = route;

            client->prev = startDepot;
            startDepot->next = client;

            for (size_t idx = 1; idx < solRoutes[r].size(); idx++)
            {
                Node *prev = client;

                client = &clients[solRoutes[r][idx]];
                client->route = route;

                client->prev = prev;
                prev->next = client;
            }

            client->next = endDepot;
            endDepot->prev = client;
        }

        route->update();
    }

    for (auto *routeOp : routeOps)
        routeOp->init(solution);
}

Solution LocalSearch::exportSolution() // const
{
    std::cout << "          LOCALSEARCH EXPORTSOLUTION Enter" << std::endl;
    std::vector<std::vector<int>> solRoutes(data.numVehicles());

    for (size_t r = 0; r < data.numVehicles(); r++)
    {
        Node *node = startDepots[r].next;

        while (!node->isDepot())
        {
            solRoutes[r].push_back(node->client);
            node = node->next;
        }
    }

    reorderRoutes(solRoutes, data);

    bool repairedConstraintsPassed = false;
    Solution sol{data, solRoutes};
    repairedConstraintsPassed = std::all_of(sol.getRoutes().begin(), sol.getRoutes().end(),
        [this](const Solution::Route &route) { return this->checkSequence(this->data, route); });
    if (repairedConstraintsPassed) {
        std::cout << "                    LOCALSEARCH EXPORTSOLUTION repairedConstraintsPassed" << std::endl;
    }
    std::cout << "          LOCALSEARCH EXPORTSOLUTION Exit" << std::endl;
    return sol;
}

bool LocalSearch::solHasValidSequences(const Solution &sol)
{
    bool isValidSequence = false;
    isValidSequence = std::all_of(sol.getRoutes().begin(), sol.getRoutes().end(),
        [this](const Solution::Route &route) { return this->checkSequence(this->data, route); });
    return isValidSequence;
}

void LocalSearch::addNodeOperator(NodeOp &op) { nodeOps.emplace_back(&op); }

void LocalSearch::addRouteOperator(RouteOp &op) { routeOps.emplace_back(&op); }

void LocalSearch::setNeighbours(Neighbours neighbours)
{
    if (neighbours.size() != data.numClients() + 1)
        throw std::runtime_error("Neighbourhood dimensions do not match.");

    for (size_t client = 0; client <= data.numClients(); ++client)
    {
        auto const beginPos = neighbours[client].begin();
        auto const endPos = neighbours[client].end();

        auto const clientPos = std::find(beginPos, endPos, client);
        auto const depotPos = std::find(beginPos, endPos, 0);

        if (clientPos != endPos || depotPos != endPos)
        {
            throw std::runtime_error("Neighbourhood of client "
                                     + std::to_string(client)
                                     + " contains itself or the depot.");
        }
    }

    auto isEmpty = [](auto const &neighbours) { return neighbours.empty(); };
    if (std::all_of(neighbours.begin(), neighbours.end(), isEmpty))
        throw std::runtime_error("Neighbourhood is empty.");

    this->neighbours = neighbours;
}

LocalSearch::Neighbours const &LocalSearch::getNeighbours() const
{
    return neighbours;
}

LocalSearch::LocalSearch(ProblemData const &data, Neighbours neighbours)
    : data(data),
      neighbours(data.numClients() + 1),
      orderNodes(data.numClients()),
      orderRoutes(data.numVehicles()),
      lastModified(data.numVehicles(), -1),
      clients(data.numClients() + 1),
      routes(data.numVehicles(), data),
      startDepots(data.numVehicles()),
      endDepots(data.numVehicles())
{
    setNeighbours(neighbours);

    std::iota(orderNodes.begin(), orderNodes.end(), 1);
    std::iota(orderRoutes.begin(), orderRoutes.end(), 0);

    for (size_t i = 0; i <= data.numClients(); i++)
        clients[i].client = i;

    for (size_t i = 0; i < data.numVehicles(); i++)
    {
        routes[i].idx = i;
        routes[i].depot = &startDepots[i];

        startDepots[i].client = 0;
        startDepots[i].route = &routes[i];

        endDepots[i].client = 0;
        endDepots[i].route = &routes[i];
    }
}

bool LocalSearch::checkSequence(ProblemData const &data, const Solution::Route &route)
{
    std::cout << "                    LOCALSEARCH CHECKSEQUENCE Enter" << std::endl;
    bool foundDelivery = false;
    bool foundBoth = false;
    bool foundSalvage = false;

    for (Client c : route) {
        bool isDelivery = (data.client(c).demandWeight || data.client(c).demandVolume);
        bool isSalvage = (data.client(c).demandSalvage == 1);
        bool isBoth = (isDelivery && isSalvage);

        std::cout << "                              LOCALSEARCH CHECKSEQUENCE Client: " << c
           << " Dem Salv: " << data.client(c).demandSalvage
           << " Dem Weig: " << data.client(c).demandWeight
           << " Dem Volu: " << data.client(c).demandVolume
           << " Salv is: " << isSalvage
           << " Salv fo: " << foundSalvage << " Both is: "
           << isBoth << " Both fo: " << foundBoth << " Del is: "
          << isDelivery << " Del fo: " << isDelivery << std::endl;

        if (isBoth && (foundBoth || foundSalvage))
        {
            std::cout << "                              LOCALSEARCH CHECKSEQUENCE Failed (isBoth && (foundBoth || foundSalvage))" << std::endl;
            std::cout << "                    LOCALSEARCH CHECKSEQUENCE Exit Selective checkSequence" << std::endl;
            return false;
        }
        if (isDelivery && (foundBoth || foundSalvage))
        {
            std::cout << "                              LOCALSEARCH CHECKSEQUENCE Failed (isDelivery && (foundBoth || foundSalvage))" << std::endl;
            std::cout << "                    LOCALSEARCH CHECKSEQUENCE Exit Selective checkSequence" << std::endl;
            return false;
        }
        if (isSalvage && foundBoth)
        {
            std::cout << "                              LOCALSEARCH CHECKSEQUENCE Failed (isSalvage && foundBoth)" << std::endl;
            std::cout << "                    LOCALSEARCH CHECKSEQUENCE Exit Selective checkSequence" << std::endl;
            return false;
        }

        if (isSalvage)
        {
            if (!foundSalvage)
                foundSalvage = true;
        }

        if (isDelivery)
        {
            if (!foundDelivery)
                foundDelivery = true;
        }

        if(isBoth)
        {
            if (!foundBoth)
                foundBoth = true;
        }

        continue;
    }
    std::cout << "                    LOCALSEARCH CHECKSEQUENCE Exit" << std::endl;
    return true;
}

void LocalSearch::reorderRoutes(std::vector<std::vector<Client>> &routes, ProblemData const &data)
{
    std::cout << "                    LOCALSEARCH REORDER Enter" << std::endl;
    std::vector<std::vector<Client>> newRoutes;

    int routeCnt=0;
    for (auto &route : routes)
    {
        std::cout << "                              LOCALSEARCH REORDER Route: " << routeCnt << std::endl;
        std::vector<Client> deliveryRoute, bothRoute, salvageRoute;

        for (const auto &client : route)
        {
            std::cout << "                                        LOCALSEARCH REORDER  Client: " << client << std::endl;
            bool isDelivery = (data.client(client).demandWeight || data.client(client).demandVolume);
            bool isSalvage = (data.client(client).demandSalvage == 1);
            bool isBoth = (isDelivery && isSalvage);

            if (isDelivery && !isBoth)
            {
                deliveryRoute.push_back(client);
            }
            else if (isBoth)
            {
                bothRoute.push_back(client);
            }
            else
            {
                salvageRoute.push_back(client);
            }
        }

        auto deliveryIter = deliveryRoute.begin();
        auto bothIter = bothRoute.begin();
        auto salvageIter = salvageRoute.begin();

        while (deliveryIter != deliveryRoute.end() || bothIter != bothRoute.end() || salvageIter != salvageRoute.end())
        {
            std::vector<Client> newRoute;

            // Add Ds
            while (deliveryIter != deliveryRoute.end())
            {
                newRoute.push_back(*deliveryIter);
                ++deliveryIter;
            }

            // Add single B if exists
            if (bothIter != bothRoute.end())
            {
                newRoute.push_back(*bothIter);
                ++bothIter;
            }

            newRoutes.push_back(newRoute);

            // If no more Bs exist, add remaining Ss to new route
            if (bothIter == bothRoute.end())
            {
                newRoute.clear();

                while (salvageIter != salvageRoute.end())
                {
                    newRoute.push_back(*salvageIter);
                    ++salvageIter;
                }

                if (!newRoute.empty())
                {
                    newRoutes.push_back(newRoute);
                }
            }
        }
        routeCnt++;
    }
    std::cout << "                    LOCALSEARCH REORDER Exit" << std::endl;
    routes = newRoutes;
}
