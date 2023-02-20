#include "LocalSearch.h"

#include <pybind11/pybind11.h>

#include <numeric>
#include <set>
#include <stdexcept>
#include <vector>

namespace py = pybind11;

void LocalSearch::search(Individual &indiv)
{
    loadIndividual(indiv);

    // Shuffling the order beforehand adds diversity to the search
    std::shuffle(orderNodes.begin(), orderNodes.end(), rng);
    std::shuffle(nodeOps.begin(), nodeOps.end(), rng);

    if (nodeOps.empty())
        throw std::logic_error("No known node operators.");

    auto neighbourhoodSize = 0;
    for (auto const client : orderNodes)
        neighbourhoodSize += neighbours[client].size();
    if (neighbourhoodSize == 0)
        throw std::runtime_error("Granular neighbourhood is empty.");

    // Caches the last time nodes were tested for modification (uses nbMoves to
    // track this). The lastModified field, in contrast, track when a route was
    // last *actually* modified.
    std::vector<int> lastTestedNodes(data.numClients() + 1, -1);
    lastModified = std::vector<int>(data.numVehicles(), 0);

    searchCompleted = false;
    nbMoves = 0;

    for (int step = 0; !searchCompleted; ++step)
    {
        searchCompleted = true;

        // Node operators are evaluated at neighbouring (U, V) pairs.
        for (auto const uClient : orderNodes)
        {
            auto *U = &clients[uClient];
            auto const lastTestedNode = lastTestedNodes[uClient];
            lastTestedNodes[uClient] = nbMoves;

            // Shuffling the neighbours in this loop should not matter much as
            // we are already randomizing the nodes U.
            for (auto const vClient : neighbours[uClient])
            {
                auto *V = &clients[vClient];

                if (lastModified[U->route->idx] > lastTestedNode
                    || lastModified[V->route->idx] > lastTestedNode)
                {
                    if (applyNodeOps(U, V))
                        continue;

                    if (p(V)->isDepot() && applyNodeOps(U, p(V)))
                        continue;
                }
            }

            // Empty route moves are not tested in the first iteration to avoid
            // increasing the fleet size too much.
            if (step > 0)
            {
                auto pred = [](auto const &route) { return route.empty(); };
                auto empty = std::find_if(routes.begin(), routes.end(), pred);

                if (empty == routes.end())
                    continue;

                if (applyNodeOps(U, empty->depot))
                    continue;
            }
        }
    }

    indiv = exportIndividual();
}

void LocalSearch::intensify(Individual &indiv)
{
    loadIndividual(indiv);

    // Shuffling the order beforehand adds diversity to the search
    std::shuffle(orderRoutes.begin(), orderRoutes.end(), rng);
    std::shuffle(routeOps.begin(), routeOps.end(), rng);

    if (routeOps.empty())
        throw std::logic_error("No known route operators.");

    std::vector<int> lastTestedRoutes(data.numVehicles(), -1);
    lastModified = std::vector<int>(data.numVehicles(), 0);

    searchCompleted = false;
    nbMoves = 0;

    while (!searchCompleted)
    {
        searchCompleted = true;

        for (int const rU : orderRoutes)
        {
            auto &U = routes[rU];

            if (U.empty())
                continue;

            auto const lastTested = lastTestedRoutes[U.idx];
            lastTestedRoutes[U.idx] = nbMoves;

            // Shuffling in this loop should not matter much as we are
            // already randomizing the routes U.
            for (int rV = 0; rV != U.idx; ++rV)
            {
                auto &V = routes[rV];

                if (V.empty())
                    continue;

                auto const lastModifiedRoute
                    = std::max(lastModified[U.idx], lastModified[V.idx]);

                if (lastModifiedRoute > lastTested && applyRouteOps(&U, &V))
                    continue;
            }
        }
    }

    indiv = exportIndividual();
}

bool LocalSearch::applyNodeOps(Node *U, Node *V)
{
    for (auto op : nodeOps)
        if (op->evaluate(U, V) < 0)
        {
            auto *routeU = U->route;  // copy pointers because the operator can
            auto *routeV = V->route;  // modify the node's route membership

            op->apply(U, V);
            update(routeU, routeV);

            return true;
        }

    return false;
}

bool LocalSearch::applyRouteOps(Route *U, Route *V)
{
    for (auto op : routeOps)
        if (op->evaluate(U, V) < 0)
        {
            op->apply(U, V);
            update(U, V);

            return true;
        }

    return false;
}

void LocalSearch::update(Route *U, Route *V)
{
    nbMoves++;
    searchCompleted = false;

    U->update();
    lastModified[U->idx] = nbMoves;

    for (auto op : routeOps)  // TODO only route operators use this (SWAP*).
        op->update(U);        //  Maybe later also expand to node ops?

    if (U != V)
    {
        V->update();
        lastModified[V->idx] = nbMoves;

        for (auto op : routeOps)
            op->update(V);
    }
}

void LocalSearch::loadIndividual(Individual const &indiv)
{
    for (size_t client = 0; client <= data.numClients(); client++)
        clients[client].tw = {&data.distanceMatrix(),
                              static_cast<int>(client),  // TODO cast
                              static_cast<int>(client),  // TODO cast
                              data.client(client).serviceDuration,
                              0,
                              data.client(client).twEarly,
                              data.client(client).twLate,
                              data.client(client).releaseTime};

    auto const &routesIndiv = indiv.getRoutes();

    for (size_t r = 0; r < data.numVehicles(); r++)
    {
        Node *startDepot = &startDepots[r];
        Node *endDepot = &endDepots[r];

        startDepot->prev = endDepot;
        startDepot->next = endDepot;

        endDepot->prev = startDepot;
        endDepot->next = startDepot;

        startDepot->tw = clients[0].tw;
        startDepot->twBefore = clients[0].tw;
        startDepot->twAfter = clients[0].tw;

        endDepot->tw = clients[0].tw;
        endDepot->twBefore = clients[0].tw;
        endDepot->twAfter = clients[0].tw;

        Route *route = &routes[r];

        if (!routesIndiv[r].empty())
        {
            Node *client = &clients[routesIndiv[r][0]];
            client->route = route;

            client->prev = startDepot;
            startDepot->next = client;

            for (int i = 1; i < static_cast<int>(routesIndiv[r].size()); i++)
            {
                Node *prev = client;

                client = &clients[routesIndiv[r][i]];
                client->route = route;

                client->prev = prev;
                prev->next = client;
            }

            client->next = endDepot;
            endDepot->prev = client;
        }

        route->update();
    }

    for (auto op : nodeOps)
        op->init(indiv);

    for (auto op : routeOps)
        op->init(indiv);
}

Individual LocalSearch::exportIndividual()
{
    std::vector<std::pair<double, int>> routePolarAngles;
    routePolarAngles.reserve(data.numVehicles());

    for (size_t r = 0; r < data.numVehicles(); r++)
        routePolarAngles.emplace_back(routes[r].angleCenter, r);

    // Empty routes have a large center angle, and thus always sort at the end
    std::sort(routePolarAngles.begin(), routePolarAngles.end());

    std::vector<std::vector<int>> indivRoutes(data.numVehicles());

    for (size_t r = 0; r < data.numVehicles(); r++)
    {
        Node *node = startDepots[routePolarAngles[r].second].next;

        while (!node->isDepot())
        {
            indivRoutes[r].push_back(node->client);
            node = node->next;
        }
    }

    return {data, penaltyManager, indivRoutes};
}

void LocalSearch::addNodeOperator(NodeOp &op) { nodeOps.emplace_back(&op); }

void LocalSearch::addRouteOperator(RouteOp &op) { routeOps.emplace_back(&op); }

void LocalSearch::setNeighbours(Neighbours neighbours)
{
    this->neighbours = neighbours;
}

LocalSearch::Neighbours LocalSearch::getNeighbours()
{
    return neighbours;
}

LocalSearch::LocalSearch(ProblemData &data,
                         PenaltyManager &penaltyManager,
                         XorShift128 &rng)
    : data(data),
      penaltyManager(penaltyManager),
      rng(rng),
      neighbours(data.numClients() + 1),
      orderNodes(data.numClients()),
      orderRoutes(data.numVehicles()),
      lastModified(data.numVehicles(), -1)
{
    std::iota(orderNodes.begin(), orderNodes.end(), 1);
    std::iota(orderRoutes.begin(), orderRoutes.end(), 0);

    clients = std::vector<Node>(data.numClients() + 1);
    routes = std::vector<Route>(data.numVehicles());
    startDepots = std::vector<Node>(data.numVehicles());
    endDepots = std::vector<Node>(data.numVehicles());

    for (size_t i = 0; i <= data.numClients(); i++)
    {
        clients[i].data = &data;
        clients[i].client = i;
    }

    for (size_t i = 0; i < data.numVehicles(); i++)
    {
        routes[i].data = &data;
        routes[i].idx = i;
        routes[i].depot = &startDepots[i];

        startDepots[i].data = &data;
        startDepots[i].client = 0;
        startDepots[i].route = &routes[i];

        startDepots[i].data = &data;
        endDepots[i].client = 0;
        endDepots[i].route = &routes[i];
    }
}
