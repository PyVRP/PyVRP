from typing import Type

from .PerturbationMethod import PerturbationMethod as PerturbationMethod
from ._perturb import DestroyOperator as DestroyOperator
from ._perturb import DestroyRepair as DestroyRepair
from ._perturb import GreedyRepair as GreedyRepair
from ._perturb import NeighbourRemoval as NeighbourRemoval
from ._perturb import RepairOperator as RepairOperator

DESTROY_OPERATORS: list[Type[DestroyOperator]] = [NeighbourRemoval]
REPAIR_OPERATORS: list[Type[RepairOperator]] = [GreedyRepair]
