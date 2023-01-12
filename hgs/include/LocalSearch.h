#ifndef LOCALSEARCH_H
#define LOCALSEARCH_H

#include "Individual.h"
#include "LocalSearchOperator.h"
#include "LocalSearchParams.h"
#include "Node.h"
#include "PenaltyManager.h"
#include "ProblemData.h"
#include "Route.h"
#include "XorShift128.h"

#include <functional>
#include <vector>

class LocalSearch
{
    using NodeOp = LocalSearchOperator<Node>;
    using RouteOp = LocalSearchOperator<Route>;

    ProblemData &data;
    PenaltyManager penaltyManager;
    XorShift128 &rng;
    LocalSearchParams params;

    // Neighborhood restrictions: For each client, list of nearby clients (size
    // nbClients + 1, but nothing stored for the depot!)
    std::vector<std::vector<int>> neighbours;

    std::vector<int> orderNodes;   // random node order used in RI operators
    std::vector<int> orderRoutes;  // random route order used in SWAP* operators

    std::vector<int> lastModified;  // tracks when routes were last modified

    std::vector<Node> clients;      // Note that clients[0] is a sentinel value
    std::vector<Node> startDepots;  // These mark the start of routes
    std::vector<Node> endDepots;    // These mark the end of routes
    std::vector<Route> routes;

    std::vector<NodeOp *> nodeOps;
    std::vector<RouteOp *> routeOps;

    int nbMoves = 0;               // Operator counter
    bool searchCompleted = false;  // No further improving move found?

    // Load an initial solution that we will attempt to improve
    void loadIndividual(Individual const &indiv);

    // Export the LS solution back into an individual
    Individual exportIndividual();

    [[nodiscard]] bool applyNodeOps(Node *U, Node *V);

    [[nodiscard]] bool applyRouteOps(Route *U, Route *V);

    // Updates solution state after an improving local search move
    void update(Route *U, Route *V);

    // Enumerates and optimally recombines subpaths of the given route
    void enumerateSubpaths(Route &U);

    // Evaluates the path before -> <nodes in sub path> -> after
    inline int evaluateSubpath(std::vector<size_t> const &subpath,
                               Node const *before,
                               Node const *after,
                               Route const &route) const;

    /**
     * Calculate, for all vertices, the correlation ('nearness') of the
     * nbGranular closest vertices.
     */
    void calculateNeighbours();

public:
    /**
     * Adds a local search operator that works on node/client pairs U and V.
     */
    void addNodeOperator(NodeOp &op) { nodeOps.emplace_back(&op); }

    /**
     * Adds a local search operator that works on route pairs U and V. These
     * operators are executed for route pairs whose circle sectors overlap.
     */
    void addRouteOperator(RouteOp &op) { routeOps.emplace_back(&op); }

    /**
     * Performs regular (node-based) local search around the given individual.
     */
    void search(Individual &indiv);

    /**
     * Performs a more intensive local search around the given individual,
     * using route-based operators and subpath enumeration.
     */
    void intensify(Individual &indiv);

    LocalSearch(ProblemData &data,
                PenaltyManager &penaltyManager,
                XorShift128 &rng,
                LocalSearchParams params = LocalSearchParams());
};

#endif
