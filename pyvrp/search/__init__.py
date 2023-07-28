from .LocalSearch import LocalSearch
from .SearchMethod import SearchMethod
from ._search import (
    Exchange10,
    Exchange11,
    Exchange20,
    Exchange21,
    Exchange22,
    Exchange30,
    Exchange31,
    Exchange32,
    Exchange33,
    MoveTwoClientsReversed,
    NodeOperator,
    RelocateStar,
    RouteOperator,
    SwapStar,
    TwoOpt,
)
from .neighbourhood import NeighbourhoodParams, compute_neighbours

NODE_OPERATORS = [
    Exchange10,
    Exchange20,
    Exchange30,
    Exchange11,
    Exchange21,
    Exchange31,
    Exchange22,
    Exchange32,
    Exchange33,
    MoveTwoClientsReversed,
    TwoOpt,
]

ROUTE_OPERATORS = [
    RelocateStar,
    SwapStar,
]
