import os
from typing import Iterable

import tomli

import pyvrp.search
from pyvrp.GeneticAlgorithm import GeneticAlgorithmParams
from pyvrp.PenaltyManager import PenaltyParams
from pyvrp.Population import PopulationParams
from pyvrp.search import (
    NODE_OPERATORS,
    ROUTE_OPERATORS,
    NeighbourhoodParams,
    NodeOperator,
    RouteOperator,
)


class Config:
    """
    Parameter configuration class for PyVRP's hybrid genetic search algorithm.
    """

    def __init__(
        self,
        genetic: GeneticAlgorithmParams = GeneticAlgorithmParams(),
        penalty: PenaltyParams = PenaltyParams(),
        population: PopulationParams = PopulationParams(),
        neighbourhood: NeighbourhoodParams = NeighbourhoodParams(),
        node_ops: Iterable[NodeOperator] = NODE_OPERATORS,
        route_ops: Iterable[RouteOperator] = ROUTE_OPERATORS,
    ):
        self._genetic = genetic
        self._penalty = penalty
        self._population = population
        self._neighbourhood = neighbourhood
        self._node_ops = node_ops
        self._route_ops = route_ops

    @property
    def genetic(self):
        return self._genetic

    @property
    def penalty(self):
        return self._penalty

    @property
    def population(self):
        return self._population

    @property
    def neighbourhood(self):
        return self._neighbourhood

    @property
    def node_ops(self):
        return self._node_ops

    @property
    def route_ops(self):
        return self._route_ops

    @classmethod
    def from_toml(cls, loc: os.PathLike):
        """
        Loads the configuration from a TOML file.
        """
        with open(loc, "rb") as fh:
            config = tomli.load(fh)

        gen_params = GeneticAlgorithmParams(**config.get("genetic", {}))
        pen_params = PenaltyParams(**config.get("penalty", {}))
        pop_params = PopulationParams(**config.get("population", {}))
        nb_params = NeighbourhoodParams(**config.get("neighbourhood", {}))
        node_ops = [getattr(pyvrp.search, op) for op in config["node_ops"]]
        route_ops = [getattr(pyvrp.search, op) for op in config["route_ops"]]

        return cls(
            gen_params, pen_params, pop_params, nb_params, node_ops, route_ops
        )
