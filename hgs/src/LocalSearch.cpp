#include "LocalSearch.h"

#include "Individual.h"
#include "Params.h"

#include <numeric>
#include <stdexcept>
#include <vector>

void LocalSearch::search(Individual &indiv)
{
    loadIndividual(indiv);

    // Shuffling the order beforehand adds diversity to the search
    std::shuffle(orderNodes.begin(), orderNodes.end(), rng);
    std::shuffle(nodeOps.begin(), nodeOps.end(), rng);

    if (nodeOps.empty())
        throw std::runtime_error("No known node operators.");

    // Caches the last time nodes were tested for modification (uses nbMoves to
    // track this). The lastModified field, in contrast, track when a route was
    // last *actually* modified.
    std::vector<int> lastTestedNodes(params.nbClients + 1, -1);
    lastModified = std::vector<int>(params.nbVehicles, 0);

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
            for (auto const vClient : params.getNeighboursOf(uClient))
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

    std::vector<int> lastTestedRoutes(params.nbVehicles, -1);
    lastModified = std::vector<int>(params.nbVehicles, 0);

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

            if (lastModified[U.idx] > lastTested)
                enumerateSubpaths(U);
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

// TODO this should be some sort of operator passed into LS, it should not be
//  defined here.
void LocalSearch::enumerateSubpaths(Route &U)
{
    auto const k = std::min(params.config.postProcessPathLength, U.size());

    if (k <= 1)  // 0 or 1 means we are either not doing anything at all (0),
        return;  // or recombining a single node (1). Neither helps.

    std::vector<size_t> path(k);

    // This postprocessing step optimally recombines all node segments of a
    // given length in each route. This recombination works by enumeration; see
    // issue #98 for details.
    for (size_t start = 1; start + k <= U.size() + 1; ++start)
    {
        auto *prev = p(U[start]);   // we process [start, start + k). So fixed
        auto *next = U[start + k];  // endpoints are p(start) and start + k

        std::iota(path.begin(), path.end(), start);
        auto currCost = evaluateSubpath(path, prev, next, U);

        while (std::next_permutation(path.begin(), path.end()))
        {
            auto const cost = evaluateSubpath(path, prev, next, U);

            if (cost < currCost)
            {
                for (auto pos : path)
                {
                    auto *node = U[pos];
                    node->insertAfter(prev);
                    prev = node;
                }

                update(&U, &U);  // it is rare to find more than one improving
                break;           // move, so we break after the first
            }
        }
    }
}

int LocalSearch::evaluateSubpath(std::vector<size_t> const &subpath,
                                 Node const *before,
                                 Node const *after,
                                 Route const &route) const
{
    auto totalDist = 0;
    auto tws = before->twBefore;
    auto from = before->client;

    // Calculates travel distance and time warp of the subpath permutation.
    for (auto &pos : subpath)
    {
        auto *to = route[pos];

        totalDist += params.dist(from, to->client);
        tws = TimeWindowSegment::merge(tws, to->tw);
        from = to->client;
    }

    totalDist += params.dist(from, after->client);
    tws = TimeWindowSegment::merge(tws, after->twAfter);

    return totalDist + params.twPenalty(tws.totalTimeWarp());
}

void LocalSearch::loadIndividual(Individual const &indiv)
{
    for (int client = 0; client <= params.nbClients; client++)
        clients[client].tw = {&params,
                              client,
                              client,
                              params.clients[client].servDur,
                              0,
                              params.clients[client].twEarly,
                              params.clients[client].twLate,
                              params.clients[client].releaseTime};

    auto const &routesIndiv = indiv.getRoutes();

    for (int r = 0; r < params.nbVehicles; r++)
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
    routePolarAngles.reserve(params.nbVehicles);

    for (int r = 0; r < params.nbVehicles; r++)
        routePolarAngles.emplace_back(routes[r].angleCenter, r);

    // Empty routes have a large center angle, and thus always sort at the end
    std::sort(routePolarAngles.begin(), routePolarAngles.end());

    std::vector<std::vector<int>> indivRoutes(params.nbVehicles);

    for (int r = 0; r < params.nbVehicles; r++)
    {
        Node *node = startDepots[routePolarAngles[r].second].next;

        while (!node->isDepot())
        {
            indivRoutes[r].push_back(node->client);
            node = node->next;
        }
    }

    return {&params, indivRoutes};
}

LocalSearch::LocalSearch(Params &params, XorShift128 &rng)
    : params(params),
      rng(rng),
      orderNodes(params.nbClients),
      orderRoutes(params.nbVehicles),
      lastModified(params.nbVehicles, -1)
{
    std::iota(orderNodes.begin(), orderNodes.end(), 1);
    std::iota(orderRoutes.begin(), orderRoutes.end(), 0);

    clients = std::vector<Node>(params.nbClients + 1);
    routes = std::vector<Route>(params.nbVehicles);
    startDepots = std::vector<Node>(params.nbVehicles);
    endDepots = std::vector<Node>(params.nbVehicles);

    for (int i = 0; i <= params.nbClients; i++)
    {
        clients[i].params = &params;
        clients[i].client = i;
    }

    for (int i = 0; i < params.nbVehicles; i++)
    {
        routes[i].params = &params;
        routes[i].idx = i;
        routes[i].depot = &startDepots[i];

        startDepots[i].params = &params;
        startDepots[i].client = 0;
        startDepots[i].route = &routes[i];

        startDepots[i].params = &params;
        endDepots[i].client = 0;
        endDepots[i].route = &routes[i];
    }
}
