#ifndef PYVRP_SEARCH_LOCALSEARCH_H
#define PYVRP_SEARCH_LOCALSEARCH_H

#include "CostEvaluator.h"
#include "LocalSearchOperator.h"
#include "PerturbationManager.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Route.h"
#include "SearchSpace.h"
#include "Solution.h"  // pyvrp::search::Solution

#include <functional>
#include <stdexcept>
#include <vector>

namespace pyvrp::search
{
class LocalSearch
{
    ProblemData const &data;

    // Stores the node-based solution representation used during LS.
    Solution solution_;

    // Manages the granular neighbourhood, promising clients, and the order in
    // which nodes and routes are searched.
    SearchSpace searchSpace_;

    // Perturbation manager that determines the size of the perturbation during
    // each LS invocation.
    PerturbationManager &perturbationManager_;

    std::vector<NodeOperator *> nodeOps;

    std::vector<int> lastTest_;    // tracks node operator evaluation
    std::vector<int> lastUpdate_;  // tracks when routes were last modified

    size_t numUpdates_ = 0;         // modification counter
    bool searchCompleted_ = false;  // No further improving move found?

    // Tests the node pair (U, V).
    bool applyNodeOps(Route::Node *U,
                      Route::Node *V,
                      CostEvaluator const &costEvaluator);

    // Tests the route pair (U, V).
    bool applyRouteOps(Route *U, Route *V, CostEvaluator const &costEvaluator);

    // Tests a move removing the given reload depot.
    void applyDepotRemovalMove(Route::Node *U,
                               CostEvaluator const &costEvaluator);

    // Tests moves involving empty routes.
    void applyEmptyRouteMoves(Route::Node *U,
                              CostEvaluator const &costEvaluator);

    // Tests moves involving missing or optional clients.
    void applyOptionalClientMoves(Route::Node *U,
                                  CostEvaluator const &costEvaluator);

    // Tests moves involving clients in client groups.
    void applyGroupMoves(Route::Node *U, CostEvaluator const &costEvaluator);

    // Updates solution state after an improving local search move.
    void update(Route *U, Route *V);

    // Performs search on the currently loaded solution.
    void search(CostEvaluator const &costEvaluator);

    // Marks missing but required clients and groups as promising, to ensure
    // they get inserted.
    void markRequiredMissingAsPromising();

public:
    /**
     * Simple data structure that tracks statistics about the number of local
     * search moves applied to the most recently improved solution.
     *
     * Attributes
     * ----------
     * num_moves
     *     Number of evaluated operator moves.
     * num_improving
     *     Number of evaluated moves that led to an objective improvement.
     * num_updates
     *     Total number of changes to the solution. This always includes the
     *     number of evaluated improving moves, but also e.g. insertion of
     *     required but missing clients.
     */
    struct Statistics
    {
        // Number of evaluated operator moves.
        size_t const numMoves;

        // Number of evaluated moves that led to an objective improvement.
        size_t const numImproving;

        // Number of times the solution has been modified in some way.
        size_t const numUpdates;
    };

    /**
     * Adds a local search operator that works on node/client pairs U and V.
     */
    void addNodeOperator(NodeOperator &op);

    /**
     * Returns the node operators in use. Note that there is no defined
     * ordering.
     */
    std::vector<NodeOperator *> const &nodeOperators() const;

    /**
     * Set neighbourhood structure to use by the local search. For each client,
     * the neighbourhood structure is a vector of nearby clients. Depots have
     * no nearby client.
     */
    void setNeighbours(SearchSpace::Neighbours neighbours);

    /**
     * Returns the current neighbourhood structure.
     */
    SearchSpace::Neighbours const &neighbours() const;

    /**
     * Returns search statistics for the currently loaded solution.
     */
    Statistics statistics() const;

    /**
     * Performs a local search around the given solution, and returns a new,
     * hopefully improved solution.
     */
    pyvrp::Solution operator()(pyvrp::Solution const &solution,
                               CostEvaluator const &costEvaluator,
                               bool exhaustive = false);

    /**
     * Shuffles the order in which the node and route pairs are evaluated, and
     * the order in which operators are applied.
     */
    void shuffle(RandomNumberGenerator &rng);

    LocalSearch(ProblemData const &data,
                SearchSpace::Neighbours neighbours,
                PerturbationManager &perturbationManager);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_LOCALSEARCH_H
