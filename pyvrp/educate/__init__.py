from pyvrp._lib.hgspy import LocalSearch, LocalSearchParams
from pyvrp._lib.hgspy.operators import (
    Exchange10,
    Exchange11,
    Exchange20,
    Exchange21,
    Exchange22,
    Exchange32,
    Exchange33,
    MoveTwoClientsReversed,
    RelocateStar,
    SwapStar,
    TwoOpt,
)

NODE_OPERATORS = [
    Exchange20,
    MoveTwoClientsReversed,
    Exchange22,
    Exchange21,
    Exchange11,
    TwoOpt,
    Exchange10,
    Exchange32,
    Exchange33,
]

ROUTE_OPERATORS = [RelocateStar, SwapStar]
