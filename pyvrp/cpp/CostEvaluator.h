#ifndef PYVRP_COSTEVALUATOR_H
#define PYVRP_COSTEVALUATOR_H

#include "Measure.h"
#include "Solution.h"

#include <cassert>
#include <concepts>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace pyvrp
{
// The following methods must be implemented for a type to be evaluatable by
// the CostEvaluator. It evaluates "cost minus profits (prizes)".
template <typename T>
concept CostEvaluatable = requires(T arg) {
    { arg.distanceCost() } -> std::same_as<Cost>;
    { arg.durationCost() } -> std::same_as<Cost>;
    { arg.fixedVehicleCost() } -> std::same_as<Cost>;
    { arg.prizes() } -> std::same_as<Cost>;
    { arg.excessLoad() } -> std::convertible_to<std::vector<Load>>;
    { arg.excessDistance() } -> std::same_as<Distance>;
    { arg.timeWarp() } -> std::same_as<Duration>;
    { arg.empty() } -> std::same_as<bool>;
    { arg.isFeasible() } -> std::same_as<bool>;
};

// If, additionally, the uncollected prizes of optional clients are implemented
// we can also take that aspect into account. The CostEvaluator then evaluates
// "cost plus uncollected profits (prizes)".
template <typename T>
concept HasUncollectedPrizes = CostEvaluatable<T> && requires(T arg) {
    { arg.uncollectedPrizes() } -> std::same_as<Cost>;
};

// The following methods must be available before a type's delta cost can be
// evaluated by the CostEvaluator.
template <typename T>
concept DeltaCostEvaluatable = requires(T arg, size_t dimension) {
    { arg.route() };
    { arg.distance() } -> std::convertible_to<std::pair<Cost, Distance>>;
    { arg.duration() } -> std::convertible_to<std::pair<Cost, Duration>>;
    { arg.excessLoad(dimension) } -> std::same_as<Load>;
};

/**
 * CostEvaluator(
 *     load_penalties: list[float],
 *     tw_penalty: float,
 *     dist_penalty: float,
 * )
 *
 * Creates a CostEvaluator instance.
 *
 * This class stores various penalty terms, and can be used to determine the
 * costs of certain constraint violations.
 *
 * Parameters
 * ----------
 * load_penalties
 *    The penalty terms (one for each load dimension) for each unit of load in
 *    excess of the vehicle capacity.
 * tw_penalty
 *    The penalty for each unit of time warp.
 * dist_penalty
 *    The penalty for each unit of distance in excess of the vehicle's maximum
 *    distance constraint.
 *
 * Raises
 * ------
 * ValueError
 *     When any of the given penalty terms are negative.
 */
class CostEvaluator
{
    std::vector<double> loadPenalties_;  // per load dimension
    double twPenalty_;
    double distPenalty_;

    /**
     * Computes the cost penalty incurred from the given excess loads. This is
     * a convenient shorthand for calling ``loadPenalty`` for each dimension.
     */
    [[nodiscard]] inline Cost
    excessLoadPenalties(std::vector<Load> const &excessLoads) const;

public:
    CostEvaluator(std::vector<double> loadPenalties,
                  double twPenalty,
                  double distPenalty);

    /**
     * Computes the total excess load penalty for the given load and vehicle
     * capacity, and dimension.
     */
    [[nodiscard]] inline Cost
    loadPenalty(Load load, Load capacity, size_t dimension) const;

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
     * Computes the excess distance penalty for the given excess distance.
     */
    [[nodiscard]] inline Cost excessDistPenalty(Distance excessDistance) const;

    /**
     * Computes a smoothed objective (penalised cost) for a given solution.
     */
    // We only expose Solution bindings to Python.
    template <CostEvaluatable T>
    [[nodiscard]] Cost penalisedCost(T const &arg) const;

    /**
     * Hand-waving some details, each solution consists of a set of non-empty
     * routes :math:`\mathcal{R}`. Each route :math:`R \in \mathcal{R}` can be
     * represented as a sequence of edges, starting and ending at a depot. A
     * route :math:`R` has an assigned vehicle type that equips the route with
     * fixed vehicle cost :math:`f_R`, and unit distance, duration and overtime
     * costs :math:`c^\text{distance}_R`, :math:`c^\text{duration}_R`,
     * :math:`c^\text{overtime}_R`, respectively. Let
     * :math:`V_R = \{i : (i, j) \in R \}` be the set of locations visited by
     * route :math:`R`, and :math:`d_R`, :math:`t_R`, and :math:`o_R` the total
     * route distance, duration, and overtime, respectively. The objective value
     * is then given by
     *
     * .. math::
     *
     *    \sum_{R \in \mathcal{R}}
     *      \left[
     *          f_R + c^\text{distance}_R d_R
     *              + c^\text{duration}_R t_R
     *              + c^\text{overtime}_R o_R
     *      \right]
     *    + \sum_{i \in V} p_i - \sum_{R \in \mathcal{R}} \sum_{i \in V_R} p_i,
     *
     * where the first part lists each route's fixed, distance, duration and
     * overtime costs, respectively, and the second part the uncollected prizes
     * of unvisited clients.
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

Cost CostEvaluator::excessLoadPenalties(
    std::vector<Load> const &excessLoads) const
{
    assert(excessLoads.size() == loadPenalties_.size());

    Cost cost = 0;
    for (size_t dim = 0; dim != loadPenalties_.size(); ++dim)
        cost += loadPenalties_[dim] * excessLoads[dim].get();

    return cost;
}

Cost CostEvaluator::loadPenalty(Load load,
                                Load capacity,
                                size_t dimension) const
{
    assert(dimension < loadPenalties_.size());
    auto const excessLoad = std::max<Load>(load - capacity, 0);
    return static_cast<Cost>(excessLoad.get() * loadPenalties_[dimension]);
}

Cost CostEvaluator::twPenalty([[maybe_unused]] Duration timeWarp) const
{
    return static_cast<Cost>(timeWarp.get() * twPenalty_);
}

Cost CostEvaluator::distPenalty(Distance distance, Distance maxDistance) const
{
    auto const excessDistance = std::max<Distance>(distance - maxDistance, 0);
    return excessDistPenalty(excessDistance);
}

Cost CostEvaluator::excessDistPenalty(Distance excessDistance) const
{
    return static_cast<Cost>(excessDistance.get() * distPenalty_);
}

template <CostEvaluatable T>
Cost CostEvaluator::penalisedCost(T const &arg) const
{
    if (arg.empty())
    {
        if constexpr (HasUncollectedPrizes<T>)
            return arg.uncollectedPrizes();
        return 0;
    }

    // Standard objective plus infeasibility-related penalty terms.
    auto const cost
        = arg.distanceCost() + arg.durationCost() + arg.fixedVehicleCost()
          + excessLoadPenalties(arg.excessLoad()) + twPenalty(arg.timeWarp())
          + distPenalty(arg.excessDistance(), 0);

    if constexpr (HasUncollectedPrizes<T>)
        // The upside of this cost versus the one based on prizes is that this
        // never goes negative. But it is a global, solution-level property:
        // for example, routes do not know about all uncollected prizes.
        return cost + arg.uncollectedPrizes();
    else
        // For routes we simply return the cost minus the collected prizes,
        // which are known at the route-level.
        return cost - arg.prizes();
}

template <CostEvaluatable T> Cost CostEvaluator::cost(T const &arg) const
{
    // Penalties are zero when the solution is feasible, so we can fall back to
    // penalised cost in that case.
    return arg.isFeasible() ? penalisedCost(arg)
                            : std::numeric_limits<Cost>::max();
}

template <bool exact, typename... Args, template <typename...> class T>
    requires(DeltaCostEvaluatable<T<Args...>>)
bool CostEvaluator::deltaCost(Cost &out, T<Args...> const &proposal) const
{
    auto const *route = proposal.route();
    if (!route->empty())
    {
        out -= route->distanceCost();
        out -= excessDistPenalty(route->excessDistance());

        out -= excessLoadPenalties(route->excessLoad());

        out -= route->durationCost();
        out -= twPenalty(route->timeWarp());
    }

    if (route->hasDistanceCost())
    {
        auto const [cost, excess] = proposal.distance();
        out += cost;
        out += excessDistPenalty(excess);
    }

    auto const &capacity = route->capacity();
    for (size_t dim = 0; dim != capacity.size(); ++dim)
    {
        if constexpr (!exact)
            if (out >= 0)
                return false;

        out += loadPenalty(proposal.excessLoad(dim), 0, dim);
    }

    if (route->hasDurationCost())
    {
        auto const [cost, timeWarp] = proposal.duration();
        out += cost;
        out += twPenalty(timeWarp);
    }

    return true;
}

template <bool exact,
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
    if (!uRoute->empty())
    {
        out -= uRoute->distanceCost();
        out -= excessDistPenalty(uRoute->excessDistance());

        out -= excessLoadPenalties(uRoute->excessLoad());

        out -= uRoute->durationCost();
        out -= twPenalty(uRoute->timeWarp());
    }

    auto const *vRoute = vProposal.route();
    if (!vRoute->empty())
    {
        out -= vRoute->distanceCost();
        out -= excessDistPenalty(vRoute->excessDistance());

        out -= excessLoadPenalties(vRoute->excessLoad());

        out -= vRoute->durationCost();
        out -= twPenalty(vRoute->timeWarp());
    }

    if (uRoute->hasDistanceCost())
    {
        auto const [cost, excess] = uProposal.distance();
        out += cost;
        out += excessDistPenalty(excess);
    }

    if (vRoute->hasDistanceCost())
    {
        auto const [cost, excess] = vProposal.distance();
        out += cost;
        out += excessDistPenalty(excess);
    }

    auto const &uCapacity = uRoute->capacity();
    for (size_t dim = 0; dim != uCapacity.size(); ++dim)
    {
        if constexpr (!exact)
            if (out >= 0)
                return false;

        out += loadPenalty(uProposal.excessLoad(dim), 0, dim);
    }

    auto const &vCapacity = vRoute->capacity();
    for (size_t dim = 0; dim != vCapacity.size(); ++dim)
    {
        if constexpr (!exact)
            if (out >= 0)
                return false;

        out += loadPenalty(vProposal.excessLoad(dim), 0, dim);
    }

    if constexpr (!exact)
        if (out >= 0)
            return false;

    if (uRoute->hasDurationCost())
    {
        auto const [cost, timeWarp] = uProposal.duration();
        out += cost;
        out += twPenalty(timeWarp);
    }

    if (vRoute->hasDurationCost())
    {
        auto const [cost, timeWarp] = vProposal.duration();
        out += cost;
        out += twPenalty(timeWarp);
    }

    return true;
}
}  // namespace pyvrp

#endif  // PYVRP_COSTEVALUATOR_H
