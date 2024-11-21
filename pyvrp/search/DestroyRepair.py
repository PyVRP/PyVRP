from typing import Optional

from pyvrp._pyvrp import (
    CostEvaluator,
    ProblemData,
    RandomNumberGenerator,
    Solution,
)


class DestroyRepair:
    def __init__(
        self,
        data: ProblemData,
        rng: RandomNumberGenerator,
        nbhd: list[list[int]],
        destroy_ops: list,
        repair_ops: list,
    ):
        self._data = data
        self._rng = rng
        self._nbhd = nbhd
        self._destroy_ops = destroy_ops
        self._repair_ops = repair_ops

    def __call__(
        self,
        solution: Solution,
        cost_evaluator: CostEvaluator,
        num_destroy: int,
        neighbours: Optional[list[list[int]]] = None,
    ) -> Solution:
        """
        This method uses the :meth:`~search` and :meth:`~intensify` methods to
        iteratively improve the given solution. First, :meth:`~search` is
        applied. Thereafter, :meth:`~intensify` is applied. This repeats until
        no further improvements are found. Finally, the improved solution is
        returned.

        Parameters
        ----------
        solution
            The solution to improve through local search.
        cost_evaluator
            Cost evaluator to use.
        neighbours
            New granular neighbourhood if passed.

        Returns
        -------
        Solution
            The improved solution. This is not the same object as the
            solution that was passed in.
        """
        rng = self._rng

        d_idx = rng.randint(len(self._destroy_ops))
        d_op = self._destroy_ops[d_idx]

        # NOTE only destroy now, LS handles repair.
        return d_op(
            self._data, solution, cost_evaluator, rng, self._nbhd, num_destroy
        )
