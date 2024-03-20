#ifndef PYVRP_COSTEVALUATOR_H
#define PYVRP_COSTEVALUATOR_H

#include "Measure.h"
#include "Solution.h"
#include "search/Route.h"

#include <concepts>
#include <limits>

namespace pyvrp
{
// The following methods must be implemented for a type to be evaluatable by
// the CostEvaluator.
template <typename T> concept CostEvaluatable = requires(T arg)
{
    // clang-format off
    { arg.distance() } -> std::same_as<Distance>;
    { arg.excessLoad() } -> std::same_as<Load>;
    { arg.excessDistance() } -> std::same_as<Distance>;
    { arg.fixedVehicleCost() }  -> std::same_as<Cost>;
    { arg.timeWarp() } -> std::same_as<Duration>;
    { arg.empty() } -> std::same_as<bool>;
    { arg.isFeasible() } -> std::same_as<bool>;
    // clang-format on
};

// If, additionally, methods related to optional clients and prize collecting
// are implemented we can also take that aspect into account. See the
// CostEvaluator implementation for details.
template <typename T>
concept PrizeCostEvaluatable = CostEvaluatable<T> and requires(T arg)
{
    // clang-format off
    { arg.uncollectedPrizes() } -> std::same_as<Cost>;
    // clang-format on
};

/**
 * CostEvaluator(load_penalty: int, tw_penalty: int, dist_penalty: int)
 *
 * Creates a CostEvaluator instance.
 *
 * This class stores various penalty terms, and can be used to determine the
 * costs of certain constraint violations.
 *
 * Parameters
 * ----------
 * load_penalty
 *    The penalty for each unit of excess load over the vehicle capacity.
 * tw_penalty
 *    The penalty for each unit of time warp.
 * dist_penalty
 *    The penalty for each unit of distance in excess of the vehicle's maximum
 *    distance constraint.
 */
class CostEvaluator
{
    Cost loadPenalty_;
    Cost twPenalty_;
    Cost distPenalty_;

public:
    CostEvaluator(Cost loadPenalty, Cost twPenalty, Cost distPenalty);

    /**
     * Computes the total excess load penalty for the given load and vehicle
     * capacity.
     */
    [[nodiscard]] inline Cost loadPenalty(Load load, Load capacity) const;

    /**
     * Computes the time warp penalty for the given time warp.
     */
    [[nodiscard]] inline Cost twPenalty(Duration timeWarp) const;

    /**
     * Computes the total excess distance penalty for the given distance.
     */
    [[nodiscard]] inline Cost distPenalty(Distance distance,
                                          Distance maxDistance) const;

    /**
     * Computes a smoothed objective (penalised cost) for a given solution.
     */
    // The docstring above is written for Python, where we only expose this
    // method for Solution.
    template <CostEvaluatable T>
    [[nodiscard]] Cost penalisedCost(T const &arg) const;

    /**
     * Hand-waving some details, each solution consists of a set of routes
     * :math:`\mathcal{R}`. Each route :math:`R \in \mathcal{R}` is a sequence
     * of edges, starting and ending at the depot. A route :math:`R` has an
     * assigned vehicle type :math:`t_R`, which has a fixed vehicle cost
     * :math:`f_{t_R}`. Let :math:`V_R = \{i : (i, j) \in R \}` be the set of
     * locations visited by route :math:`R`. The objective value is then given
     * by
     *
     * .. math::
     *
     *    \sum_{R \in \mathcal{R}} \left[ f_{t_R}
     *          + \sum_{(i, j) \in R} d_{ij} \right]
     *    + \sum_{i \in V} p_i - \sum_{R \in \mathcal{R}} \sum_{i \in V_R} p_i,
     *
     * where the first part lists the vehicle and distance costs, and the
     * second part the uncollected prizes of unvisited clients.
     *
     * .. note::
     *
     *    The above cost computation only holds for feasible solutions. If the
     *    solution argument is *infeasible*, we return a very large number.
     *    If that is not what you want, consider calling :meth:`penalised_cost`
     *    instead.
     */
    // The docstring above is written for Python, where we only expose this
    // method for Solution.
    template <CostEvaluatable T> [[nodiscard]] Cost cost(T const &arg) const;

    /**
     * TODO
     */
    template <bool exact = false, typename... Args>
    bool deltaCost(search::Route::Proposal<Args...> const &proposal,
                   Cost &deltaCost) const;

    /**
     * TODO
     */
    template <bool exact = false, typename... uArgs, typename... vArgs>
    bool deltaCost(search::Route::Proposal<uArgs...> const &uProposal,
                   search::Route::Proposal<vArgs...> const &vProposal,
                   Cost &deltaCost) const;
};

Cost CostEvaluator::loadPenalty(Load load, Load capacity) const
{
    auto const excessLoad = std::max<Load>(load - capacity, 0);
    return static_cast<Cost>(excessLoad) * loadPenalty_;
}

Cost CostEvaluator::twPenalty([[maybe_unused]] Duration timeWarp) const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return 0;
#else
    return static_cast<Cost>(timeWarp) * twPenalty_;
#endif
}

Cost CostEvaluator::distPenalty(Distance distance, Distance maxDistance) const
{
    auto const excessDistance = std::max<Distance>(distance - maxDistance, 0);
    return static_cast<Cost>(excessDistance) * distPenalty_;
}

template <CostEvaluatable T>
Cost CostEvaluator::penalisedCost(T const &arg) const
{
    // Standard objective plus penalty terms for infeasibilities.
    auto const cost = static_cast<Cost>(arg.distance())
                      + (!arg.empty() ? arg.fixedVehicleCost() : 0)
                      + loadPenalty(arg.excessLoad(), 0)
                      + twPenalty(arg.timeWarp())
                      + distPenalty(arg.excessDistance(), 0);

    if constexpr (PrizeCostEvaluatable<T>)
        return cost + arg.uncollectedPrizes();

    return cost;
}

template <CostEvaluatable T> Cost CostEvaluator::cost(T const &arg) const
{
    // Penalties are zero when the solution is feasible, so we can fall back to
    // penalised cost in that case.
    return arg.isFeasible() ? penalisedCost(arg)
                            : std::numeric_limits<Cost>::max();
}

template <bool exact = false, typename... Args>
bool CostEvaluator::deltaCost(search::Route::Proposal<Args...> const &proposal,
                              Cost &deltaCost) const
{
    auto const *route = proposal.route();
    auto const dist = proposal.distanceSegment();

    deltaCost += static_cast<Cost>(dist.distance());
    deltaCost += distPenalty(dist.distance(), route->maxDistance());

    deltaCost -= static_cast<Cost>(route->distance());
    deltaCost -= distPenalty(route->distance(), route->maxDistance());

    deltaCost -= loadPenalty(route->load(), route->capacity());
    deltaCost -= twPenalty(route->timeWarp());

    if constexpr (!exact)
    {
        if (deltaCost >= 0)
            return false;
    }

    auto const load = proposal.loadSegment();
    deltaCost += loadPenalty(load.load(), route->capacity());

    auto const duration = proposal.durationSegment();
    deltaCost += twPenalty(duration.timeWarp(route->maxDuration()));

    return true;
}

template <bool exact = false, typename... uArgs, typename... vArgs>
bool CostEvaluator::deltaCost(
    search::Route::Proposal<uArgs...> const &uProposal,
    search::Route::Proposal<vArgs...> const &vProposal,
    Cost &deltaCost) const
{
    auto const *uRoute = uProposal.route();
    auto const *vRoute = vProposal.route();

    auto const uDist = uProposal.distanceSegment();
    deltaCost += static_cast<Cost>(uDist.distance());
    deltaCost += distPenalty(uDist.distance(), uRoute->maxDistance());

    deltaCost -= static_cast<Cost>(uRoute->distance());
    deltaCost -= distPenalty(uRoute->distance(), uRoute->maxDistance());

    deltaCost -= loadPenalty(uRoute->load(), uRoute->capacity());
    deltaCost -= twPenalty(uRoute->timeWarp());

    auto const vDist = vProposal.distanceSegment();
    deltaCost += static_cast<Cost>(vDist.distance());
    deltaCost += distPenalty(vDist.distance(), vRoute->maxDistance());

    deltaCost -= static_cast<Cost>(vRoute->distance());
    deltaCost -= distPenalty(vRoute->distance(), vRoute->maxDistance());

    deltaCost -= loadPenalty(vRoute->load(), vRoute->capacity());
    deltaCost -= twPenalty(vRoute->timeWarp());

    if constexpr (!exact)
    {
        if (deltaCost >= 0)
            return false;
    }

    auto const uLoad = uProposal.loadSegment();
    deltaCost += loadPenalty(uLoad.load(), uRoute->capacity());

    auto const uDuration = uProposal.durationSegment();
    deltaCost += twPenalty(uDuration.timeWarp(uRoute->maxDuration()));

    auto const vLoad = vProposal.loadSegment();
    deltaCost += loadPenalty(vLoad.load(), vRoute->capacity());

    auto const vDuration = vProposal.durationSegment();
    deltaCost += twPenalty(vDuration.timeWarp(vRoute->maxDuration()));

    return true;
}

}  // namespace pyvrp

#endif  // PYVRP_COSTEVALUATOR_H
