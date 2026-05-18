# PyVRP Solver Architecture

This document describes how PyVRP works, both algorithmically and in terms of software
architecture. It is aimed at new developers who want to understand the core principles
and extend or modify the solver.

## Table of contents

1. [Big picture](#1-big-picture)
2. [Software architecture: Python/C++ split](#2-software-architecture-pythonc-split)
3. [Algorithm: Iterated Local Search with late acceptance](#3-algorithm-iterated-local-search-with-late-acceptance)
4. [Cost evaluation and penalty management](#4-cost-evaluation-and-penalty-management)
5. [Local search internals](#5-local-search-internals)
6. [Granular neighbourhood](#6-granular-neighbourhood)
7. [Perturbation](#7-perturbation)
8. [Search operators](#8-search-operators)
9. [Solution representations](#9-solution-representations)
10. [Entry points and configuration](#10-entry-points-and-configuration)

---

## 1. Big picture

PyVRP solves vehicle routing problems (VRPs) using an **iterated local search (ILS)**
algorithm. The main loop repeatedly:

1. Perturbs the current solution (insert/remove a handful of clients)
2. Applies a fast local search to repair and improve it
3. Decides whether to accept the result using a **late acceptance hill-climbing (LAHC)**
   criterion
4. Updates the global best if improved

Performance-critical work (local search operators, route cost tracking, delta
evaluation) is done in C++. The outer algorithm loop, penalty management, and
configuration are in Python.

The codebase is historically described as a "hybrid genetic search" (HGS), after
the original Vidal (2022) paper that inspired it, but the current implementation does
not use a population or crossover. It is purely ILS.

---

## 2. Software architecture: Python/C++ split

```
pyvrp/
├── IteratedLocalSearch.py   # main algorithm loop
├── solve.py                 # public solve() entry point, wires components
├── PenaltyManager.py        # adaptive penalty weights
├── Model.py                 # user-friendly problem definition API
├── search/
│   ├── LocalSearch.py       # thin Python wrapper around C++ LocalSearch
│   ├── neighbourhood.py     # granular neighbourhood computation
│   └── _search.so           # compiled C++ extension (operators, LocalSearch)
└── cpp/
    ├── CostEvaluator.h/.cpp # cost computation + delta evaluation
    ├── Solution.h/.cpp      # "full" solution representation (for statistics)
    ├── Route.h/.cpp         # route statistics (distance, load, time warp, ...)
    └── search/
        ├── LocalSearch.h/.cpp       # local search loop
        ├── SearchSpace.h/.cpp       # neighbourhood + promising-client tracking
        ├── PerturbationManager.h/.cpp
        ├── Exchange.h               # (N,M)-exchange operator (template)
        ├── SwapTails.h/.cpp
        ├── RelocateWithDepot.h/.cpp
        ├── RemoveOptional.h/.cpp
        ├── RemoveAdjacentDepot.h/.cpp
        └── Route.h/.cpp             # mutable route for in-place LS moves
```

**Two Route classes** exist deliberately:

| Class | Namespace | Purpose |
|---|---|---|
| `pyvrp::Route` | `pyvrp/cpp/Route.h` | Immutable, full statistics. Used in `Solution` for reporting. |
| `pyvrp::search::Route` | `pyvrp/cpp/search/Route.h` | Mutable, supports fast in-place node moves and delta evaluation via `Proposal`. |

When the local search loads a solution, it converts `pyvrp::Solution` into the mutable
`search::Route` representation. On exit it converts back.

---

## 3. Algorithm: Iterated Local Search with late acceptance

**File:** `pyvrp/IteratedLocalSearch.py`

### Main loop (`IteratedLocalSearch.run`)

```
best = curr = initial_solution

while not stop:
    cand = search(curr)            # perturb + local search
    pm.register(cand)              # update penalty weights

    if cand < best:
        best = cand
        if exhaustive_on_best:
            cand = search(cand, exhaustive=True)  # squeeze more improvement
            best = cand if feasible

    # Late acceptance hill-climbing acceptance
    late = history[oldest]
    if cand_cost < late_cost or cand_cost < curr_cost:
        curr = cand

    # Only update history when current improves on the stored value
    if curr_cost < late_cost:
        history.append(curr)
    else:
        history.skip()

    if iters_no_improvement == limit:
        curr = best          # restart from best
        history.clear()
```

### Late acceptance hill-climbing (LAHC)

Standard hill-climbing only accepts improving moves. Simulated annealing accepts
worsening moves probabilistically with a temperature schedule. LAHC is simpler: it
accepts a candidate if it is better than the solution from `history_length` iterations
ago (default 300). This allows the search to escape shallow local optima without
needing a temperature schedule.

The implementation uses the two enhancements from Burke & Bykov (2017, §4.2):

1. **Dual acceptance**: accept if better than the late solution *or* the current
   solution (whichever is more permissive).
2. **Selective history update**: only write to the history when the current solution
   improves on the value already stored there, otherwise just advance the pointer
   (`history.skip()`). This avoids degrading the history with worse solutions.

`RingBuffer` (`pyvrp/RingBuffer.py`) implements the circular buffer. `peek()` returns
the oldest entry (about to be overwritten), and `append()`/`skip()` both advance the
write pointer.

### Restart

After `num_iters_no_improvement` (default 150,000) iterations without a new global
best, the algorithm resets `curr = best` and clears the history. This is a hard
restart, not a warm restart, so the history pressure that was pushing the search
in a certain direction is released.

---

## 4. Cost evaluation and penalty management

### Objective function

The true objective (for feasible solutions) is defined in `CostEvaluator::cost`:

```
∑_routes [ fixed_vehicle_cost
         + unit_distance_cost × route_distance
         + unit_duration_cost × route_duration
         + unit_elevation_cost × (load × elevation_gain)  ]
+ uncollected prizes (for optional clients)
```

The objective is **integer-valued** throughout. Users are expected to scale their data
(e.g., multiply distances by 100) before passing it to the solver.

### Penalised cost

During the search, infeasible solutions (violating capacity, time windows, or max
distance) are allowed and guided back to feasibility through penalty terms:

```
penalised_cost = objective
               + load_penalty  × excess_load    (per load dimension)
               + tw_penalty    × time_warp
               + dist_penalty  × excess_distance
```

`CostEvaluator::penalisedCost()` computes this. `CostEvaluator::cost()` returns
`INT_MAX` for infeasible solutions.

### Adaptive penalty management (`PenaltyManager`)

**File:** `pyvrp/PenaltyManager.py`

The penalty weights are not fixed. Every `solutions_between_updates` (default 500)
registered solutions, the manager checks what fraction were feasible. The target
fraction is `target_feasible` (default 65%). If too few feasible solutions have been
seen, penalties increase (`× 1.5`); if too many, they decrease (`× 0.95`). A small
`feas_tolerance` (5%) band avoids oscillating updates.

This keeps the search in a productive zone: most solutions are infeasible (exploring
the space freely) but feasible solutions appear regularly (keeping the incumbent useful).

Initial penalty values are scaled to the problem instance: one unit of constraint
violation costs roughly the same as one average edge traversal.

---

## 5. Local search internals

**Files:** `pyvrp/search/LocalSearch.py` (Python wrapper),
`pyvrp/cpp/search/LocalSearch.h/.cpp` (implementation)

### How a search invocation works (`LocalSearch::operator()`)

1. Load the solution into the mutable `search::Solution` representation.
2. Initialise all operators (resets their statistics).
3. Either mark all clients as promising (exhaustive mode) or run the perturbation
   manager to perturb the solution and mark affected clients as promising.
4. Run the search loop.
5. Unload the solution back into a `pyvrp::Solution` and return it.

### The search loop (`LocalSearch::search`)

The loop runs until no improving move is found (i.e., one full pass with no change):

```
for each client U in randomised order:
    insertRequired(U)               # enforce required clients

    if U is not promising: skip

    applyUnaryOps(U)                # single-route operators
    applyOptionalClientMoves(U)     # insert/remove optional clients
    applyGroupMoves(U)              # handle mutually-exclusive client groups

    if U not in solution: continue

    for each V in neighbourhood(U):
        if V not in solution: continue
        if route(U) or route(V) updated since last test:
            applyBinaryOps(U, V)    # two-route operators
            applyBinaryOps(U, depot-before-V)

    if step > 0:
        applyEmptyRouteMoves(U)     # try moving U to an empty route
```

The `step > 0` guard on empty route moves is intentional: in the first pass the search
avoids opening new routes, which would inflate vehicle count. Only after exhausting
intra/inter-route moves does it consider empty routes.

### Promising clients

`SearchSpace` maintains a bitset of "promising" clients. Only promising clients are
evaluated in the inner loop. When a move is applied, the affected clients and their
immediate neighbours are marked promising. This makes the search **incremental**: it
re-evaluates only the parts of the solution that might have changed, not everything.

In exhaustive mode, all clients are marked promising at the start. In the default
(non-exhaustive) mode, only clients touched by perturbation are initially promising.

### Operator ordering and tie-breaking

Before each local search call, `LocalSearch::shuffle` randomises:
- The order of unary operators
- The order of binary operators
- The client evaluation order
- The vehicle type order (for empty route moves)

This prevents systematic bias and ensures different parts of the search space are
explored on different invocations.

### Change tracking

Two integer counters are used to avoid redundant evaluations:
- `lastTest_[client]`: the `numUpdates_` value when client U was last tested
- `lastUpdate_[route]`: the `numUpdates_` value when a route was last modified

The binary operator loop skips pair (U, V) if neither `route(U)` nor `route(V)` has
been modified since U was last tested. This is correct: if nothing changed in either
route, the outcome of the evaluation would be identical.

---

## 6. Granular neighbourhood

**File:** `pyvrp/search/neighbourhood.py`

An O(n²) full neighbourhood search is too slow for large instances. PyVRP uses a
**granular neighbourhood**: each client has a fixed list of its `k` closest neighbours
(default `k = 50`). Binary operators only consider (U, V) pairs where V is in U's
neighbourhood list. This reduces the search from O(n²) to O(kn).

### Proximity metric

Proximity between clients i and j is not purely geographic distance. It accounts for:

- **Edge cost**: `unit_distance_cost × distance(i,j) + unit_duration_cost × duration(i,j)`
  (taking the minimum across all vehicle types)
- **Wait time penalty**: if j's time window opens much later than i's, inserting j
  directly after i forces the vehicle to wait
- **Time warp penalty**: if it is impossible to reach j from i in time, that segment
  causes time warp

```python
proximity(i,j) = edge_cost(i,j)
               - prize(j)
               + w_wait × max(early_j - duration(i,j) - service_i - late_i, 0)
               + w_tw   × max(early_i + service_i + duration(i,j) - late_j, 0)
```

This means two geographically close clients with incompatible time windows will be
far apart in proximity, and the local search will not waste time trying to route
them together.

The top-`k` neighbours are computed once before the search starts and held fixed
throughout.

---

## 7. Perturbation

**File:** `pyvrp/cpp/search/PerturbationManager.cpp`

Each local search call (non-exhaustive) begins with a perturbation step. This serves
two purposes:
1. Escape the local optimum that the previous search settled into.
2. Seed the promising-client set for the subsequent search (so the search knows where
   to focus).

The perturbation picks a random number of moves between `min_perturbations` (1) and
`max_perturbations` (25). It walks through clients in randomised order. For each
selected client U:

- If U is **in** the solution: remove U and its neighbours (REMOVE perturbation)
- If U is **not** in the solution: insert U and its neighbours (INSERT perturbation)

Clients already touched by a previous perturbation move in the same step are skipped.
After perturbation, all affected clients are marked promising.

This is the mechanism that replaces crossover from the original HGS. Rather than
combining two parent solutions, the perturbation injects structured noise into a
single solution.

---

## 8. Search operators

**Files:** `pyvrp/cpp/search/Exchange.h`, `SwapTails.h/.cpp`,
`RelocateWithDepot.h/.cpp`, `RemoveOptional.h/.cpp`, `RemoveAdjacentDepot.h/.cpp`

### Operator types

There are two base types, defined in `LocalSearchOperator.h`:

- **`UnaryOperator`**: takes a single node U. Used for moves within a single route.
- **`BinaryOperator`**: takes two nodes U and V. Used for moves involving two routes
  (or two positions within one route).

Both expose `evaluate(args, cost_evaluator) -> (delta_cost, should_apply)` and
`apply(args)`. The `evaluate` method is allowed to return early with an inexact
(non-negative) delta if it determines the move cannot be improving — this is a
correctness-safe performance optimisation.

### Default operators

| Operator | Type | Description |
|---|---|---|
| `Exchange10` | Binary | Move 1 client from U's route into V's route (relocate) |
| `Exchange20` | Binary | Move 2 consecutive clients from U's route into V's route |
| `Exchange11` | Binary | Swap 1 client from U with 1 client from V |
| `Exchange21` | Binary | Swap 2 consecutive clients from U with 1 client from V |
| `Exchange22` | Binary | Swap 2 consecutive clients from U with 2 from V |
| `SwapTails` | Binary | Swap the tail segments of two routes from U and V onward |
| `RelocateWithDepot` | Binary | Move a client along with its associated reload depot visit |
| `RemoveAdjacentDepot` | Unary | Remove redundant consecutive depot visits in a route |
| `RemoveOptional` | Unary | Remove an optional client if doing so reduces cost |

### Exchange template

`Exchange<N, M>` is a C++ template covering a family of operators. N clients starting
at U are swapped with M clients starting at V. When M == 0 it is a pure relocate
(one-sided move); when N == M it is a symmetric swap. The template is instantiated for
all combinations up to `Exchange33`.

### Delta evaluation

Move evaluation computes the **change** in penalised cost, not the total cost. This is
critical for performance: recomputing the full cost of two routes after every candidate
move would be far too slow.

`CostEvaluator::deltaCost()` takes a `Route::Proposal` — a description of what the
route would look like after the move, expressed as a sequence of pre-computed segments.
The delta is computed as:

```
delta = proposed_cost(U) + proposed_cost(V)
      - current_cost(U)  - current_cost(V)
```

In the non-exact variant, the evaluator returns early (with `return false`) as soon as
the running sum becomes non-negative (i.e., the move cannot be improving). This
short-circuit eliminates most load and duration evaluations for non-improving moves.

---

## 9. Solution representations

### `pyvrp::Solution` (the "outer" solution)

**File:** `pyvrp/cpp/Solution.h`

The canonical solution type, used as the interface between the Python algorithm layer
and the C++ local search. It is immutable once constructed. Stores a vector of
`pyvrp::Route` objects, each of which holds full route statistics (distance, duration,
load, time warp, elevation cost, etc.) computed at construction time.

This is the object that `IteratedLocalSearch` stores as `best` and `curr`, and the
one returned to the user.

### `pyvrp::search::Solution` and `pyvrp::search::Route` (the "inner" solution)

**Files:** `pyvrp/cpp/search/Solution.h`, `pyvrp/cpp/search/Route.h`

Used exclusively inside the C++ local search. The solution is a linked list of nodes
(each node knows its predecessor, successor, and route). Routes support O(1) node
insertion and removal. Statistics are cached and only recomputed on demand via
`Route::update()` — the local search calls this after applying a move.

The `Route::Proposal` template aggregates pre-cached segment data to enable efficient
delta computation without actually modifying the route.

---

## 10. Entry points and configuration

### `solve()` (`pyvrp/solve.py`)

The primary public API. It:
1. Constructs a `RandomNumberGenerator` with the given seed.
2. Computes the granular neighbourhood.
3. Builds a `LocalSearch` object and adds all configured operators.
4. Initialises `PenaltyManager` with data-scaled penalties.
5. Constructs an initial solution (random → exhaustive LS) unless one is provided.
6. Runs `IteratedLocalSearch`.

### `SolveParams`

A dataclass that bundles all configuration:

| Field | Controls |
|---|---|
| `ils: IteratedLocalSearchParams` | History length, restart threshold, exhaustive-on-best |
| `penalty: PenaltyParams` | Penalty update frequency, increase/decrease factors, target feasibility |
| `neighbourhood: NeighbourhoodParams` | Number of neighbours, proximity weights |
| `operators` | Which operators to use (default: all nine) |
| `perturbation: PerturbationParams` | Min/max perturbations per iteration |

Parameters can be loaded from a TOML file via `SolveParams.from_file()`.

### Stopping criteria (`pyvrp/stop/`)

`solve()` accepts any `StoppingCriterion`, which is a callable `(best_cost) -> bool`.
Built-in criteria:
- `MaxRuntime(seconds)`: wall-clock time limit
- `MaxIterations(n)`: iteration count limit
- `NoImprovement(n)`: stop after n iterations with no best improvement
- `FirstFeasible`: stop as soon as a feasible solution is found
- `MultipleCriteria([...])`: logical OR of multiple criteria
