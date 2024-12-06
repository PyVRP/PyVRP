#ifndef PYVRP_COSTEVALUATOR_H
#define PYVRP_COSTEVALUATOR_H

#include "Measure.h"
#include "Solution.h"

#include <concepts>
#include <limits>

namespace pyvrp
{
// The following methods must be implemented for a type to be evaluatable by
// the CostEvaluator.
template <typename T>
concept CostEvaluatable = requires(T arg) {
    { arg.distanceCost() } -> std::same_as<Cost>;
    { arg.durationCost() } -> std::same_as<Cost>;
    { arg.fixedVehicleCost() } -> std::same_as<Cost>;
    { arg.excessLoad() } -> std::convertible_to<std::vector<Load>>;
    { arg.excessDistance() } -> std::same_as<Distance>;
    { arg.timeWarp() } -> std::same_as<Duration>;
    { arg.empty() } -> std::same_as<bool>;
    { arg.isFeasible() } -> std::same_as<bool>;
};

// If, additionally, methods related to optional clients and prize collecting
// are implemented we can also take that aspect into account. See the
// CostEvaluator implementation for details.
template <typename T>
concept PrizeCostEvaluatable = CostEvaluatable<T> && requires(T arg) {
    { arg.uncollectedPrizes() } -> std::same_as<Cost>;
};

// The following methods must be available before a type's delta cost can be
// evaluated by the CostEvaluator.
template <typename T>
concept DeltaCostEvaluatable = requires(T arg, size_t dimension) {
    { arg.route() };
    { arg.distanceSegment() };
    { arg.durationSegment() };
    { arg.loadSegment(dimension) };
};

/**
 * CostEvaluator(load_penalty: float, tw_penalty: float, dist_penalty: float)
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
    double loadPenalty_;
    double twPenalty_;
    double distPenalty_;

public:
    CostEvaluator(double loadPenalty, double twPenalty, double distPenalty);

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
     * Hand-waving some details, each solution consists of a set of non-empty
     * routes :math:`\mathcal{R}`. Each route :math:`R \in \mathcal{R}` is a
     * sequence of edges, starting and ending at a depot. Each route :math:`R`
     * has an assigned vehicle type, through which the route is equipped with a
     * fixed vehicle cost :math:`f_R`, and unit distance and duration costs
     * :math:`c^\text{distance}_R` and :math:`c^\text{duration}_R`,
     * respectively. Let :math:`V_R = \{i : (i, j) \in R \}` be the set of
     * locations visited by route :math:`R`, and :math:`d_R` and :math:`t_R`
     * the total route distance and duration, respectively. The objective value
     * is then given by
     *
     * .. math::
     *
     *    \sum_{R \in \mathcal{R}}
     *      \left[
     *          f_R + c^\text{distance}_R d_R + c^\text{duration}_R t_R
     *      \right]
     *    + \sum_{i \in V} p_i - \sum_{R \in \mathcal{R}} \sum_{i \in V_R} p_i,
     *
     * where the first part lists each route's fixed, distance and duration
     * costs, respectively, and the second part the uncollected prizes of
     * unvisited clients.
     *
     * .. note::
     *
     *    The above cost computation only holds for feasible solutions. If the
     *    solution argument is *infeasible*, we return a very large number.
     *    If that is not what you want, consider calling :meth:`penalised_cost`
     *    instead.
     */
    // The docstring above is written for Python, where we only expose this
    // method for the Solution class.
    template <CostEvaluatable T> [[nodiscard]] Cost cost(T const &arg) const;

    /**
     * Evaluates the cost delta of the given route proposal, and writes the
     * resulting cost delta to the ``out`` parameter. The evaluation can be
     * exact, if the relevant template argument is set. Else it may shortcut
     * once it determines that the proposal does not constitute an improving
     * move. Optionally, several aspects of the evaluation may be skipped.
     *
     * The return value indicates whether the evaluation was exact or not.
     */
    template <bool exact = false,
              bool skipLoad = false,
              typename... Args,
              template <typename...>
              class T>
        requires(DeltaCostEvaluatable<T<Args...>>)
    bool deltaCost(Cost &out, T<Args...> const &proposal) const;

    /**
     * Evaluates the cost delta of the given route proposals, and writes the
     * resulting cost delta to the ``out`` parameter. The evaluation can be
     * exact, if the relevant template argument is set. Else it may shortcut
     * once it determines that the proposals do not constitute an improving
     * move. Optionally, several aspects of the evaluation may be skipped.
     *
     * The return value indicates whether the evaluation was exact or not.
     */
    template <bool exact = false,
              bool skipLoad = false,
              typename... uArgs,
              typename... vArgs,
              template <typename...>
              class T>
        requires(DeltaCostEvaluatable<T<uArgs...>>
                 && DeltaCostEvaluatable<T<vArgs...>>)
    bool deltaCost(Cost &out,
                   T<uArgs...> const &uProposal,
                   T<vArgs...> const &vProposal) const;
};

Cost CostEvaluator::loadPenalty(Load load, Load capacity) const
{
    auto const excessLoad = std::max<Load>(load - capacity, 0);
    return static_cast<Cost>(excessLoad.get() * loadPenalty_);
}

Cost CostEvaluator::twPenalty([[maybe_unused]] Duration timeWarp) const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return 0;
#else
    return static_cast<Cost>(timeWarp.get() * twPenalty_);
#endif
}

Cost CostEvaluator::distPenalty(Distance distance, Distance maxDistance) const
{
    auto const excessDistance = std::max<Distance>(distance - maxDistance, 0);
    return static_cast<Cost>(excessDistance.get() * distPenalty_);
}

template <CostEvaluatable T>
Cost CostEvaluator::penalisedCost(T const &arg) const
{
    // Standard objective plus infeasibility-related penalty terms.
    auto cost = arg.distanceCost() + arg.durationCost()
                + (!arg.empty() ? arg.fixedVehicleCost() : 0)
                + twPenalty(arg.timeWarp())
                + distPenalty(arg.excessDistance(), 0);

    for (auto const excess : arg.excessLoad())
        cost += loadPenalty(excess, 0);

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

template <bool exact,
          bool skipLoad,
          typename... Args,
          template <typename...>
          class T>
    requires(DeltaCostEvaluatable<T<Args...>>)
bool CostEvaluator::deltaCost(Cost &out, T<Args...> const &proposal) const
{
    auto const *route = proposal.route();

    out -= route->distanceCost();
    out -= distPenalty(route->distance(), route->maxDistance());

    if constexpr (!skipLoad)
        for (auto const excess : route->excessLoad())
            out -= loadPenalty(excess, 0);

    out -= route->durationCost();
    out -= twPenalty(route->timeWarp());

    auto const dist = proposal.distanceSegment();
    out += route->unitDistanceCost() * static_cast<Cost>(dist.distance());
    out += distPenalty(dist.distance(), route->maxDistance());

    if constexpr (!exact)
        if (out >= 0)
            return false;

    if constexpr (!skipLoad)
    {
        auto const &capacity = route->capacity();
        for (size_t dim = 0; dim != capacity.size(); ++dim)
            out += loadPenalty(proposal.loadSegment(dim).load(), capacity[dim]);
    }

    auto const duration = proposal.durationSegment();
    out += route->unitDurationCost() * static_cast<Cost>(duration.duration());
    out += twPenalty(duration.timeWarp(route->maxDuration()));

    return true;
}

template <bool exact,
          bool skipLoad,
          typename... uArgs,
          typename... vArgs,
          template <typename...>
          class T>
    requires(DeltaCostEvaluatable<T<uArgs...>>
             && DeltaCostEvaluatable<T<vArgs...>>)
bool CostEvaluator::deltaCost(Cost &out,
                              T<uArgs...> const &uProposal,
                              T<vArgs...> const &vProposal) const
{
    auto const *uRoute = uProposal.route();
    auto const *vRoute = vProposal.route();

    out -= uRoute->distanceCost();
    out -= distPenalty(uRoute->distance(), uRoute->maxDistance());

    out -= vRoute->distanceCost();
    out -= distPenalty(vRoute->distance(), vRoute->maxDistance());

    if constexpr (!skipLoad)
    {
        for (auto const excess : uRoute->excessLoad())
            out -= loadPenalty(excess, 0);

        for (auto const excess : vRoute->excessLoad())
            out -= loadPenalty(excess, 0);
    }

    out -= uRoute->durationCost();
    out -= twPenalty(uRoute->timeWarp());

    out -= vRoute->durationCost();
    out -= twPenalty(vRoute->timeWarp());

    auto const uDist = uProposal.distanceSegment();
    out += uRoute->unitDistanceCost() * static_cast<Cost>(uDist.distance());
    out += distPenalty(uDist.distance(), uRoute->maxDistance());

    auto const vDist = vProposal.distanceSegment();
    out += vRoute->unitDistanceCost() * static_cast<Cost>(vDist.distance());
    out += distPenalty(vDist.distance(), vRoute->maxDistance());

    if constexpr (!exact)
        if (out >= 0)
            return false;

    if constexpr (!skipLoad)
    {
        auto const &uCapacity = uRoute->capacity();
        for (size_t dim = 0; dim != uCapacity.size(); ++dim)
            out += loadPenalty(uProposal.loadSegment(dim).load(),
                               uCapacity[dim]);

        auto const &vCapacity = vRoute->capacity();
        for (size_t dim = 0; dim != vCapacity.size(); ++dim)
            out += loadPenalty(vProposal.loadSegment(dim).load(),
                               vCapacity[dim]);
    }

    if constexpr (!exact)
        if (out >= 0)
            return false;

    auto const uDuration = uProposal.durationSegment();
    out += uRoute->unitDurationCost() * static_cast<Cost>(uDuration.duration());
    out += twPenalty(uDuration.timeWarp(uRoute->maxDuration()));

    auto const vDuration = vProposal.durationSegment();
    out += vRoute->unitDurationCost() * static_cast<Cost>(vDuration.duration());
    out += twPenalty(vDuration.timeWarp(vRoute->maxDuration()));

    return true;
}
}  // namespace pyvrp

#endif  // PYVRP_COSTEVALUATOR_H
