#ifndef PYVRP_SEARCH_LOCALSEARCH_H
#define PYVRP_SEARCH_LOCALSEARCH_H

#include "CostEvaluator.h"
#include "DynamicBitset.h"
#include "LocalSearchOperator.h"
#include "PerturbationOperator.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Route.h"
#include "Solution.h"

#include <functional>
#include <stdexcept>
#include <vector>

namespace pyvrp::search
{
class LocalSearch
{
public:
    /**
     * Simple data structure that tracks statistics about the number of local
     * search moves applied to the most recently improved solution.
     *
     * Attributes
     * ----------
     * num_moves
     *     Number of evaluated node and route operator moves.
     * num_improving
     *     Number of evaluated moves that led to an objective improvement.
     * num_updates
     *     Total number of changes to the solution. This always includes the
     *     number of evaluated improving moves, but also e.g. insertion of
     *     required but missing clients.
     */
    struct Statistics
    {
        // Number of evaluated moves, that is, number of evaluations of a node
        // or route operator.
        size_t numMoves = 0;

        // Number of evaluated moves that led to an objective improvement.
        size_t numImproving = 0;

        // Number of times the solution has been modified in some way.
        size_t numUpdates = 0;
    };

private:
    using Neighbours = std::vector<std::vector<size_t>>;

    ProblemData const &data;

    // Neighborhood restrictions: list of nearby clients for each client (size
    // numLocations, but nothing is stored for the depots!)
    Neighbours neighbours_;

    size_t numPerturbations_ = 0;  // number of perturbations to apply

    std::vector<size_t> orderNodes;         // node order used by LS::search
    std::vector<std::pair<size_t, size_t>>  // vehicle type order (incl. offset)
        orderVehTypes;                      // used by LS::applyEmptyRouteMoves

    std::vector<int> lastTestedNodes;  // tracks node operator evaluation
    std::vector<int> lastUpdated;      // tracks when routes were last modified
    DynamicBitset promising;           // tracks which nodes are likely to be
                                       // improved by node ops

    std::vector<Route::Node> nodes;
    std::vector<Route> routes;

    std::vector<NodeOperator *> nodeOps;
    std::vector<PerturbationOperator *> perturbOps;

    size_t numUpdates_ = 0;         // modification counter
    bool searchCompleted_ = false;  // No further improving move found?

    // Load an initial solution that we will attempt to improve.
    void loadSolution(Solution const &solution);

    // Export the LS solution back into a solution.
    Solution exportSolution() const;

    // Tests the node pair (U, V).
    bool applyNodeOps(Route::Node *U,
                      Route::Node *V,
                      CostEvaluator const &costEvaluator);

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

    // Marks the given node and its direct neighbours as promising.
    void markPromising(Route::Node const *U);

    // Updates solution state after an improving local search move.
    void update(Route *U, Route *V);

    // Performs search on the currently loaded solution.
    void search(CostEvaluator const &costEvaluator);

    // Performs perturb on the currently loaded solution.
    void perturb(CostEvaluator const &costEvaluator);

    // Evaluate inserting U after one of its neighbours or a random empty route.
    // Applies the move if it's improving or required for feasibility.
    void
    insert(Route::Node *U, CostEvaluator const &costEvaluator, bool required);

public:
    /**
     * Adds a local search operator that works on node/client pairs U and V.
     */
    void addNodeOperator(NodeOperator &op);

    /**
     * Adds a perturbation operator.
     */
    void addPerturbationOperator(PerturbationOperator &op);

    /**
     * Returns the node operators in use. Note that there is no defined
     * ordering.
     */
    std::vector<NodeOperator *> const &nodeOperators() const;

    /**
     * Returns the perturbation operators in use. Note that there is no defined
     * ordering.
     */
    std::vector<PerturbationOperator *> const &perturbationOperators() const;

    /**
     * Set neighbourhood structure to use by the local search. For each client,
     * the neighbourhood structure is a vector of nearby clients. Depots have
     * no nearby client.
     */
    void setNeighbours(Neighbours neighbours);

    /**
     * Returns the current neighbourhood structure.
     */
    Neighbours const &neighbours() const;

    /**
     * Sets the number of perturbations to apply for all perturbation
     * operators.
     */
    void setNumPerturbations(size_t numPerturbations);

    /**
     * Returns the number of perturbations to apply for all perturbation
     * operators.
     */
    size_t numPerturbations() const;

    /**
     * Returns search statistics for the currently loaded solution.
     */
    Statistics statistics() const;

    /**
     * Calls ``perturb()`` followed by ``search()``.
     */
    Solution operator()(Solution const &solution,
                        CostEvaluator const &costEvaluator);

    /**
     * Performs regular (node-based) local search around the given solution,
     * and returns a new, hopefully improved solution.
     */
    Solution search(Solution const &solution,
                    CostEvaluator const &costEvaluator);

    /**
     * Performs a perturbation step around the given solution, and returns a
     * new, modified solution.
     */
    Solution perturb(Solution const &solution,
                     CostEvaluator const &costEvaluator);

    /**
     * Shuffles the order in which the node and route pairs are evaluated, and
     * the order in which operators are applied.
     */
    void shuffle(RandomNumberGenerator &rng);

    LocalSearch(ProblemData const &data, Neighbours neighbours);
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_LOCALSEARCH_H
