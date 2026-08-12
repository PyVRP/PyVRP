from typing import Type

from .LocalSearch import LocalSearch as LocalSearch
from .SearchMethod import SearchMethod as SearchMethod
from ._search import BinaryOperator as BinaryOperator
from ._search import InsertOptionalClient as InsertOptionalClient
from ._search import InsertOptionalShipment as InsertOptionalShipment
from ._search import NeighbourhoodParams as NeighbourhoodParams
from ._search import PerturbationManager as PerturbationManager
from ._search import PerturbationParams as PerturbationParams
from ._search import Relocate1 as Relocate1
from ._search import Relocate2 as Relocate2
from ._search import Relocate3 as Relocate3
from ._search import RelocateAlternative as RelocateAlternative
from ._search import RelocateDelivery as RelocateDelivery
from ._search import RelocatePickup as RelocatePickup
from ._search import RelocateShipment as RelocateShipment
from ._search import RelocateWithDepot as RelocateWithDepot
from ._search import RemoveAdjacentDepot as RemoveAdjacentDepot
from ._search import RemoveOptionalClient as RemoveOptionalClient
from ._search import RemoveOptionalShipment as RemoveOptionalShipment
from ._search import ReplaceGroup as ReplaceGroup
from ._search import ReplaceOptionalClient as ReplaceOptionalClient
from ._search import ReplaceOptionalShipment as ReplaceOptionalShipment
from ._search import Swap11 as Swap11
from ._search import Swap21 as Swap21
from ._search import Swap22 as Swap22
from ._search import Swap31 as Swap31
from ._search import Swap32 as Swap32
from ._search import Swap33 as Swap33
from ._search import SwapTails as SwapTails
from ._search import UnaryOperator as UnaryOperator
from .neighbourhood import compute_neighbours as compute_neighbours

OPERATORS: list[Type[UnaryOperator | BinaryOperator]] = [
    Relocate1,
    Relocate2,
    Swap11,
    Swap21,
    Swap22,
    SwapTails,
    RelocateAlternative,
    RelocatePickup,
    RelocateDelivery,
    RelocateWithDepot,
    RemoveAdjacentDepot,
    RemoveOptionalClient,
    InsertOptionalClient,
    ReplaceOptionalClient,
    RemoveOptionalShipment,
    InsertOptionalShipment,
    ReplaceOptionalShipment,
    ReplaceGroup,
    RelocateShipment,
]
