#include "LocalSearch.h"

#include <algorithm>
#include <numeric>
#include <set>
#include <stdexcept>
#include <vector>

Individual LocalSearch::search(Individual &individual,
                               CostEvaluator const &costEvaluator)
{
    loadIndividual(individual);

    // Shuffling the order beforehand adds diversity to the search
    std::shuffle(orderNodes.begin(), orderNodes.end(), rng);
    std::shuffle(nodeOps.begin(), nodeOps.end(), rng);

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
            // TODO only loop over U that are not currently in virtual route?

            auto *U = &clients[uClient];
            auto const lastTestedNode = lastTestedNodes[uClient];
            lastTestedNodes[uClient] = numMoves;

            // Shuffling the neighbours in this loop should not matter much as
            // we are already randomizing the nodes U.
            for (auto const vClient : neighbours[uClient])
            {
                auto *V = &clients[vClient];

                if (lastModified[U->route->idx] > lastTestedNode
                    || lastModified[V->route->idx] > lastTestedNode)
                {
                    if (applyNodeOps(U, V, costEvaluator))
                        continue;

                    if (p(V)->isDepot() && applyNodeOps(U, p(V), costEvaluator))
                        continue;
                }
            }

            // Test inserting U into the virtual route. U is then no longer
            // visited.
            if (applyNodeOps(U, routes.back().depot, costEvaluator))
                continue;

            // Empty route moves are not tested in the first iteration to avoid
            // increasing the fleet size too much.
            if (step > 0)
            {
                auto pred = [](auto const &route) { return route.empty(); };
                auto empty = std::find_if(routes.begin(), routes.end(), pred);

                if (empty == routes.end())
                    continue;

                if (applyNodeOps(U, empty->depot, costEvaluator))
                    continue;
            }
        }
    }

    return exportIndividual();
}

Individual LocalSearch::intensify(Individual &individual,
                                  CostEvaluator const &costEvaluator,
                                  int overlapToleranceDegrees)
{
    loadIndividual(individual);

    auto const overlapTolerance = overlapToleranceDegrees * 65536;

    // Shuffling the order beforehand adds diversity to the search
    std::shuffle(orderRoutes.begin(), orderRoutes.end(), rng);
    std::shuffle(routeOps.begin(), routeOps.end(), rng);

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

    return exportIndividual();
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

void LocalSearch::loadIndividual(Individual const &individual)
{
    for (size_t client = 0; client <= data.numClients(); client++)
    {
        clients[client].tw = {static_cast<int>(client),  // TODO cast
                              static_cast<int>(client),  // TODO cast
                              data.client(client).serviceDuration,
                              0,
                              data.client(client).twEarly,
                              data.client(client).twLate};

        clients[client].route = nullptr;
    }

    auto const &routesIndiv = individual.getRoutes();

    for (size_t r = 0; r <= data.numVehicles(); r++)
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

        if (r < data.numVehicles())
        {
            route->isVirtual = false;

            if (!routesIndiv[r].empty())
            {
                Node *client = &clients[routesIndiv[r][0]];
                client->route = route;

                client->prev = startDepot;
                startDepot->next = client;

                for (size_t idx = 1; idx < routesIndiv[r].size(); idx++)
                {
                    Node *prev = client;

                    client = &clients[routesIndiv[r][idx]];
                    client->route = route;

                    client->prev = prev;
                    prev->next = client;
                }

                client->next = endDepot;
                endDepot->prev = client;
            }
        }
        else
        {
            // Insert unrouted clients into special route
            route->isVirtual = true;
            Node *prev = nullptr;

            for (auto &client : clients)
                if (!client.route)
                {
                    client.route = route;

                    if (!prev)
                    {
                        client.prev = startDepot;
                        startDepot->next = &client;
                    }
                    else
                    {
                        client.prev = prev;
                        prev->next = &client;
                    }

                    prev = &client;
                }

            if (prev)
            {
                prev->next = endDepot;
                endDepot->prev = prev;
            }
        }

        route->update();
    }

    for (auto *routeOp : routeOps)
        routeOp->init(individual);
}

Individual LocalSearch::exportIndividual()
{
    std::vector<std::vector<int>> indivRoutes(data.numVehicles());

    for (size_t r = 0; r < data.numVehicles(); r++)
    {
        Node *node = startDepots[r].next;

        while (!node->isDepot())
        {
            indivRoutes[r].push_back(node->client);
            node = node->next;
        }
    }

    return {data, indivRoutes};
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

LocalSearch::Neighbours LocalSearch::getNeighbours() { return neighbours; }

LocalSearch::LocalSearch(ProblemData &data,
                         XorShift128 &rng,
                         Neighbours neighbours)
    : data(data),
      rng(rng),
      neighbours(data.numClients() + 1),
      orderNodes(data.numClients()),
      orderRoutes(data.numVehicles()),
      lastModified(data.numVehicles(), -1),
      clients(data.numClients() + 1),
      routes(data.numVehicles() + 1, data),  // +1 for special route
      startDepots(data.numVehicles() + 1),
      endDepots(data.numVehicles() + 1)
{
    setNeighbours(neighbours);

    std::iota(orderNodes.begin(), orderNodes.end(), 1);
    std::iota(orderRoutes.begin(), orderRoutes.end(), 0);

    for (size_t i = 0; i <= data.numClients(); i++)
        clients[i].client = i;

    for (size_t i = 0; i <= data.numVehicles(); i++)
    {
        routes[i].idx = i;
        routes[i].depot = &startDepots[i];

        startDepots[i].client = 0;
        startDepots[i].route = &routes[i];

        endDepots[i].client = 0;
        endDepots[i].route = &routes[i];
    }
}
