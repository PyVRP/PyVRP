import pathlib
from typing import Callable, List, Optional, Union

from pyvrp._ProblemData import Client, ProblemData
from pyvrp.read import read


class Model:
    """
    A modelling interface for vehicle routing problems.
    """

    def __init__(self):
        self._clients: List[Client] = []
        self._depots: List[Client] = []
        self._data: Optional[ProblemData] = None

    @classmethod
    def read(
        cls,
        where: Union[str, pathlib.Path],
        instance_format: str,
        round_func: Union[str, Callable],
    ) -> "Model":
        data = read(where, instance_format, round_func)

        self = Model()
        self._clients = [
            data.client(idx) for idx in range(1, data.num_clients)
        ]
        self._depots = [data.client(0)]
        self._data = data

        return self

    def add_client(
        self,
        x: int,
        y: int,
        demand: int = 0,
        service_duration: int = 0,
        tw_early: int = 0,
        tw_late: int = 0,
        prize: int = 0,
        required: bool = True,
    ):
        self._clients.append(
            Client(
                x,
                y,
                demand,
                service_duration,
                tw_early,
                tw_late,
                prize,
                required,
            )
        )

    def add_depot(self, x: int, y: int, tw_early: int = 0, tw_late: int = 0):
        if len(self._depots) > 1:
            msg = "PyVRP does not currently support multi-depot VRPs."
            raise ValueError(msg)

        self._depots.append(Client(x, y, tw_early=tw_early, tw_late=tw_late))

    def add_edge(self):
        pass

    def update(self):
        pass

    def solve(self):
        if self._data is None:
            self.update()

        # TODO
