#ifndef LOCALSEARCH_H
#define LOCALSEARCH_H

#include "CostEvaluator.h"
#include "Individual.h"
#include "LocalSearchOperator.h"
#include "Node.h"
#include "ProblemData.h"
#include "Route.h"
#include "XorShift128.h"

#include <functional>
#include <stdexcept>
#include <vector>

class LocalSearch
{
    using NodeOp = LocalSearchOperator<Node>;
    using RouteOp = LocalSearchOperator<Route>;
    using Neighbours = std::vector<std::vector<int>>;

    ProblemData &data;
    XorShift128 &rng;

    // Neighborhood restrictions: For each client, list of nearby clients (size
    // nbClients + 1, but nothing stored for the depot!)
    Neighbours neighbours;

    std::vector<int> orderNodes;   // random node order used in RI operators
    std::vector<int> orderRoutes;  // random route order used in SWAP* operators

    std::vector<int> lastModified;  // tracks when routes were last modified

    std::vector<Node> clients;  // Note that clients[0] is a sentinel value
    std::vector<Route> routes;
    std::vector<Node> startDepots;  // These mark the start of routes
    std::vector<Node> endDepots;    // These mark the end of routes

    std::vector<NodeOp *> nodeOps;
    std::vector<RouteOp *> routeOps;

    int numMoves = 0;              // Operator counter
    bool searchCompleted = false;  // No further improving move found?

    // Load an initial solution that we will attempt to improve
    void loadIndividual(Individual const &individual);

    // Export the LS solution back into an individual
    Individual exportIndividual();

    // Tests the node pair (U, V).
    bool applyNodeOps(Node *U, Node *V, CostEvaluator const &costEvaluator);

    // Tests the route pair (U, V).
    bool applyRouteOps(Route *U, Route *V, CostEvaluator const &costEvaluator);

    // Updates solution state after an improving local search move.
    void update(Route *U, Route *V);

    // Test inserting U after V. Called if U is not currently in the solution.
    void maybeInsert(Node *U, Node *V, CostEvaluator const &costEvaluator);

    // Test removing U from the solution. Called when U can be removed.
    void maybeRemove(Node *U, CostEvaluator const &costEvaluator);

public:
    /**
     * Adds a local search operator that works on node/client pairs U and V.
     */
    void addNodeOperator(NodeOp &op);

    /**
     * Adds a local search operator that works on route pairs U and V. These
     * operators are executed for route pairs whose circle sectors overlap.
     */
    void addRouteOperator(RouteOp &op);

    /**
     * Set neighbourhood structure to use by the local search. For each client,
     * the neighbourhood structure is a vector of nearby clients. The depot has
     * no nearby client.
     */
    void setNeighbours(Neighbours neighbours);

    /**
     * @return The neighbourhood structure currently in use.
     */
    Neighbours getNeighbours();

    /**
     * Performs regular (node-based) local search around the given individual,
     * and returns a new, hopefully improved individual.
     */
    Individual search(Individual &individual,
                      CostEvaluator const &costEvaluator);

    /**
     * Performs a more intensive local search around the given individual,
     * using route-based operators and subpath enumeration. Returns a new,
     * hopefully improved individual.
     */
    Individual intensify(Individual &individual,
                         CostEvaluator const &costEvaluator,
                         int overlapToleranceDegrees = 0);

    LocalSearch(ProblemData &data, XorShift128 &rng, Neighbours neighbours);
};

#endif
