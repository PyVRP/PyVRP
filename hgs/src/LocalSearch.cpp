#include "LocalSearch.h"

#include "Individual.h"
#include "ProblemData.h"

#include <numeric>
#include <set>
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
    std::vector<int> lastTestedNodes(data.nbClients + 1, -1);
    lastModified = std::vector<int>(data.nbVehicles, 0);

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

    std::vector<int> lastTestedRoutes(data.nbVehicles, -1);
    lastModified = std::vector<int>(data.nbVehicles, 0);

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
    auto const k = std::min(data.config.postProcessPathLength, U.size());

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

        totalDist += data.dist(from, to->client);
        tws = TimeWindowSegment::merge(tws, to->tw);
        from = to->client;
    }

    totalDist += data.dist(from, after->client);
    tws = TimeWindowSegment::merge(tws, after->twAfter);

    return totalDist + data.pManager.twPenalty(tws.totalTimeWarp());
}

void LocalSearch::calculateNeighbours()
{
    // TODO clean up this method / rethink proximity determination
    auto proximities
        = std::vector<std::vector<std::pair<int, int>>>(data.nbClients + 1);

    for (int i = 1; i <= data.nbClients; i++)  // exclude depot
    {
        auto &proximity = proximities[i];

        for (int j = 1; j <= data.nbClients; j++)  // exclude depot
        {
            if (i == j)  // exclude the current client
                continue;

            // Compute proximity using Eq. 4 in Vidal 2012. The proximity is
            // computed by the distance, min. wait time and min. time warp
            // going from either i -> j or j -> i, whichever is the least.
            int const maxRelease = std::max(data.clients[i].releaseTime,
                                            data.clients[j].releaseTime);

            // Proximity from j to i
            int const waitTime1 = data.clients[i].twEarly - data.dist(j, i)
                                  - data.clients[j].servDur
                                  - data.clients[j].twLate;

            int const earliestArrival1 = std::max(maxRelease + data.dist(0, j),
                                                  data.clients[j].twEarly);

            int const timeWarp1 = earliestArrival1 + data.clients[j].servDur
                                  + data.dist(j, i) - data.clients[i].twLate;

            int const prox1
                = data.dist(j, i)
                  + data.config.weightWaitTime * std::max(0, waitTime1)
                  + data.config.weightTimeWarp * std::max(0, timeWarp1);

            // Proximity from i to j
            int const waitTime2 = data.clients[j].twEarly - data.dist(i, j)
                                  - data.clients[i].servDur
                                  - data.clients[i].twLate;
            int const earliestArrival2 = std::max(maxRelease + data.dist(0, i),
                                                  data.clients[i].twEarly);
            int const timeWarp2 = earliestArrival2 + data.clients[i].servDur
                                  + data.dist(i, j) - data.clients[j].twLate;
            int const prox2
                = data.dist(i, j)
                  + data.config.weightWaitTime * std::max(0, waitTime2)
                  + data.config.weightTimeWarp * std::max(0, timeWarp2);

            proximity.emplace_back(std::min(prox1, prox2), j);
        }

        std::sort(proximity.begin(), proximity.end());
    }

    // First create a set of correlated vertices for each vertex (where the
    // depot is not taken into account)
    std::vector<std::set<int>> set(data.nbClients + 1);
    size_t const granularity = std::min(
        data.config.nbGranular, static_cast<size_t>(data.nbClients) - 1);

    for (int i = 1; i <= data.nbClients; i++)  // again exclude depot
    {
        auto const &orderProximity = proximities[i];

        for (size_t j = 0; j != granularity; ++j)
            set[i].insert(orderProximity[j].second);
    }

    for (int i = 1; i <= data.nbClients; i++)
        for (int x : set[i])
            neighbours[i].push_back(x);
}

void LocalSearch::loadIndividual(Individual const &indiv)
{
    for (int client = 0; client <= data.nbClients; client++)
        clients[client].tw = {&data.dist_,
                              client,
                              client,
                              data.clients[client].servDur,
                              0,
                              data.clients[client].twEarly,
                              data.clients[client].twLate,
                              data.clients[client].releaseTime};

    auto const &routesIndiv = indiv.getRoutes();

    for (int r = 0; r < data.nbVehicles; r++)
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
    routePolarAngles.reserve(data.nbVehicles);

    for (int r = 0; r < data.nbVehicles; r++)
        routePolarAngles.emplace_back(routes[r].angleCenter, r);

    // Empty routes have a large center angle, and thus always sort at the end
    std::sort(routePolarAngles.begin(), routePolarAngles.end());

    std::vector<std::vector<int>> indivRoutes(data.nbVehicles);

    for (int r = 0; r < data.nbVehicles; r++)
    {
        Node *node = startDepots[routePolarAngles[r].second].next;

        while (!node->isDepot())
        {
            indivRoutes[r].push_back(node->client);
            node = node->next;
        }
    }

    return {&data, indivRoutes};
}

LocalSearch::LocalSearch(ProblemData &data, XorShift128 &rng)
    : data(data),
      rng(rng),
      neighbours(data.nbClients + 1),
      orderNodes(data.nbClients),
      orderRoutes(data.nbVehicles),
      lastModified(data.nbVehicles, -1)
{
    std::iota(orderNodes.begin(), orderNodes.end(), 1);
    std::iota(orderRoutes.begin(), orderRoutes.end(), 0);

    clients = std::vector<Node>(data.nbClients + 1);
    routes = std::vector<Route>(data.nbVehicles);
    startDepots = std::vector<Node>(data.nbVehicles);
    endDepots = std::vector<Node>(data.nbVehicles);

    calculateNeighbours();

    for (int i = 0; i <= data.nbClients; i++)
    {
        clients[i].data = &data;
        clients[i].client = i;
    }

    for (int i = 0; i < data.nbVehicles; i++)
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
