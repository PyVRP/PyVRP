#ifndef PYVRP_DIVERSITY_H
#define PYVRP_DIVERSITY_H

#include "ProblemData.h"
#include "Solution.h"

#include <functional>

namespace pyvrp::diversity
{
typedef std::function<double(Solution const &, Solution const &)>
    DiversityMeasure;

/**
 * Computes the symmetric broken pairs distance (BPD) between the given two
 * solutions. This function determines whether each client in the problem
 * shares neighbours between the first and second solution. If not, the
 * client is part of a 'broken pair': a link that is part of one solution,
 * but not of the other.
 *
 * Formally, given two solutions :math:`f` and :math:`s`, let :math:`p_f(i)`
 * and :math:`p_s(i)` be the preceding client (or depot) of client
 * :math:`i = 1, \ldots, n` in :math:`f` and :math:`s`, respectively.
 * Similarly define :math:`s_f(i)` and :math:`s_s(i)` for the succeeding
 * client (or depot). Then, we have
 *
 * .. math::
 *
 *    \text{BPD}(f, s) = \frac{
 *        \sum_{i = 1}^n 1_{p_f(i) \ne p_s(i)} + 1_{s_f(i) \ne s_s(i)}
 *    }{2n}.
 *
 * .. note::
 *
 *    Note that our definition is directed: a route ``[1, 2, 3, 4]`` is
 *    considered completely different from a route ``[4, 3, 2, 1]``.
 *
 * Parameters
 * ----------
 * first
 *     First solution.
 * second
 *     Second solution.
 *
 * Returns
 * -------
 * float
 *     A value in [0, 1] that indicates the percentage of broken links between
 *     the two solutions. A value near one suggests the solutions are
 *     maximally diverse, a value of zero indicates they are the same.
 */
double brokenPairsDistance(Solution const &first, Solution const &second);
}  // namespace pyvrp::diversity

#endif  // PYVRP_DIVERSITY_H
