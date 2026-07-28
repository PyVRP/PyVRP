#include "ShipmentRuinAndRecreate.h"

#include "DeliverySegment.h"
#include "PickupSegment.h"

#include <algorithm>
#include <cassert>
#include <limits>

using pyvrp::search::ShipmentRuinAndRecreate;

namespace
{
// Experimental constants. See the plan in the session notes; these would be
// PerturbationParams fields in a real implementation.
constexpr double RUIN_PROBABILITY = 0.25;
constexpr size_t MIN_RUIN = 8;
constexpr size_t MAX_RUIN = 15;
constexpr double LARGE_RUIN_PROBABILITY = 0.05;
constexpr size_t MAX_LARGE_RUIN = 30;
constexpr size_t NUM_RELATED = 100;
constexpr size_t NUM_CANDIDATE_ROUTES = 6;
}  // namespace

ShipmentRuinAndRecreate::ShipmentRuinAndRecreate(ProblemData const &data)
    : data(data), rng_(42)
{
    auto const numShip = data.numShipments();
    if (numShip == 0 || data.numClients() != 0)
        return;  // experimental: pure-shipment instances only

    auto const &dists = data.distanceMatrix(0);
    auto const loc = [&](size_t act)
    {
        return act < numShip ? data.shipment(act).pickup.location
                             : data.shipment(act - numShip).delivery.location;
    };

    auto const numActs = 2 * numShip;
    related_.resize(numActs);

    std::vector<std::pair<double, size_t>> scored;
    scored.reserve(numActs);

    for (size_t frm = 0; frm != numActs; ++frm)
    {
        scored.clear();
        auto const from = loc(frm);
        for (size_t to = 0; to != numActs; ++to)
        {
            if (to == frm || (to % numShip) == (frm % numShip))
                continue;  // skip self and own sibling

            auto const other = loc(to);
            auto const score = static_cast<double>(dists(from, other))
                               + static_cast<double>(dists(other, from));
            scored.emplace_back(score, to);
        }

        auto const keep = std::min(NUM_RELATED, scored.size());
        std::partial_sort(scored.begin(), scored.begin() + keep, scored.end());

        related_[frm].reserve(keep);
        for (size_t idx = 0; idx != keep; ++idx)
            related_[frm].push_back(scored[idx].second);
    }
}

void ShipmentRuinAndRecreate::reseed(RandomNumberGenerator &rng)
{
    // Drawing from rng shifts the caller's random stream, so we only do so
    // when this perturbation is actually active. Instances without shipments
    // then keep the exact stream they had before.
    if (related_.empty())
        return;

    rng_ = RandomNumberGenerator(rng());
}

bool ShipmentRuinAndRecreate::apply(Solution &solution,
                                    SearchSpace &searchSpace,
                                    CostEvaluator const &costEvaluator) const
{
    if (related_.empty())
        return false;

    auto const numShip = data.numShipments();
    auto const draw = static_cast<double>(rng_()) / rng_.max();
    if (draw >= RUIN_PROBABILITY)
        return false;

    // ---- select a seed activity belonging to an assigned shipment ----
    std::vector<size_t> assigned;
    assigned.reserve(2 * numShip);
    for (size_t ship = 0; ship != numShip; ++ship)
        if (solution.shipments[ship].first.route())
        {
            assigned.push_back(ship);            // pickup activity
            assigned.push_back(numShip + ship);  // delivery activity
        }

    if (assigned.size() < 2 * MIN_RUIN)
        return false;

    auto const seedAct = assigned[rng_.randint(assigned.size())];
    auto const seedShip = seedAct % numShip;
    auto *seedRoute = solution.shipments[seedShip].first.route();

    // ---- collect two or three related routes ----
    std::vector<Route *> routes = {seedRoute};
    auto const wanted = 2 + rng_.randint(2);  // two or three
    for (auto const act : related_[seedAct])
    {
        if (routes.size() >= wanted)
            break;

        auto *route = solution.shipments[act % numShip].first.route();
        if (route
            && std::find(routes.begin(), routes.end(), route) == routes.end())
            routes.push_back(route);
    }

    if (routes.size() < 2)
        return false;

    // ---- how many shipments to remove ----
    size_t available = 0;
    for (auto const *route : routes)
        available += route->numShipments();

    auto const large
        = static_cast<double>(rng_()) / rng_.max() < LARGE_RUIN_PROBABILITY;
    auto target = large ? MAX_RUIN + 1 + rng_.randint(MAX_LARGE_RUIN - MAX_RUIN)
                        : MIN_RUIN + rng_.randint(MAX_RUIN - MIN_RUIN + 1);
    target = std::min(target, available);
    if (target < MIN_RUIN)
        return false;

    // ---- grow a contiguous string in each route until the quota is met ----
    std::vector<bool> chosen(numShip, false);
    size_t numChosen = 0;

    for (size_t idx = 0; idx != routes.size() && numChosen < target; ++idx)
    {
        auto *route = routes[idx];
        if (route->size() <= 2)
            continue;

        // Quota for this route: share the remainder over the routes left.
        auto const left = routes.size() - idx;
        auto quota = (target - numChosen + left - 1) / left;
        quota = std::min(quota, route->numShipments());

        auto const numNodes = route->size() - 2;  // excluding both depots
        auto pos = 1 + rng_.randint(numNodes);    // anchor
        auto lo = pos, hi = pos;
        size_t taken = 0;

        while (taken < quota)
        {
            auto *node = (*route)[pos];
            if (!node->isDepot())
            {
                auto const ship = node->idx();
                if (!chosen[ship])
                {
                    chosen[ship] = true;
                    ++taken;
                    ++numChosen;
                }
            }

            bool const canLeft = lo > 1;
            bool const canRight = hi < route->size() - 2;
            if (!canLeft && !canRight)
                break;

            if (canRight && (!canLeft || rng_.randint(2)))
                pos = ++hi;
            else
                pos = --lo;
        }
    }

    if (numChosen < MIN_RUIN)
        return false;

    // ---- remove the chosen shipments ----
    // The removed shipments are left unassigned. LocalSearch's
    // ensureStructuralFeasibility reinserts them through Solution::insert,
    // exactly as the legacy perturbation relies on.
    searchSpace.unmarkAllPromising();

    for (size_t ship = 0; ship != numShip; ++ship)
    {
        if (!chosen[ship])
            continue;

        auto *pickup = &solution.shipments[ship].first;
        auto *delivery = &solution.shipments[ship].second;
        auto *route = pickup->route();
        assert(route && delivery->route() == route);

        // Mark while still assigned; markPromising asserts on that. The
        // survivors either side of each removed node become newly adjacent,
        // so they are promising too.
        searchSpace.markPromising(pickup);
        searchSpace.markPromising(delivery);

        route->remove(delivery->pos());  // descending, so the pickup position
        route->remove(pickup->pos());    // is still valid here
    }

    for (auto *route : routes)
    {
        route->update();
        if (route->empty())
            route->clear();
    }

    recreate(solution, searchSpace, costEvaluator);
    return true;
}

void ShipmentRuinAndRecreate::recreate(Solution &solution,
                                       SearchSpace &searchSpace,
                                       CostEvaluator const &costEvaluator) const
{
    if (related_.empty())
        return;

    auto const numShip = data.numShipments();

    // Insertion order biases the result, since each insertion changes what
    // later shipments see. Shuffle rather than always going by index.
    std::vector<size_t> pending;
    for (size_t ship = 0; ship != numShip; ++ship)
        if (!solution.shipments[ship].first.route())
            pending.push_back(ship);

    rng_.shuffle(pending.begin(), pending.end());

    for (auto const ship : pending)
    {
        auto *pickup = &solution.shipments[ship].first;
        auto *delivery = &solution.shipments[ship].second;

        // Candidate routes: those holding the shipment's nearest activities,
        // from both its pickup and delivery rows, plus one empty route.
        std::vector<Route *> routes;

        auto const &pickRow = related_[ship];
        auto const &delivRow = related_[numShip + ship];
        auto const rows = std::max(pickRow.size(), delivRow.size());

        // Alternate by rank so the delivery row actually contributes; draining
        // the pickup row first fills every slot before it is ever consulted.
        for (size_t rank = 0; rank != rows; ++rank)
        {
            if (routes.size() >= NUM_CANDIDATE_ROUTES)
                break;

            for (auto const *row : {&pickRow, &delivRow})
            {
                if (rank >= row->size()
                    || routes.size() >= NUM_CANDIDATE_ROUTES)
                    continue;

                auto const other = (*row)[rank];
                auto *route = solution.shipments[other % numShip].first.route();
                if (route
                    && std::find(routes.begin(), routes.end(), route)
                           == routes.end())
                    routes.push_back(route);
            }
        }

        for (auto &route : solution.routes)
            if (route.empty())
            {
                routes.push_back(&route);
                break;
            }

        if (routes.empty())
            continue;  // ensureStructuralFeasibility will pick this up

        auto const prize = data.shipment(ship).prize;
        Cost bestCost = std::numeric_limits<Cost>::max();
        Route *bestRoute = nullptr;
        size_t bestPick = 0, bestDeliv = 0;

        for (auto *route : routes)
        {
            auto const last = route->size() - 1;
            auto const fixed = route->empty() ? route->fixedVehicleCost() : 0;

            for (size_t i = 0; i != last; ++i)
                for (size_t j = i; j != last; ++j)
                {
                    Cost delta = fixed - prize;
                    if (i == j)
                        costEvaluator.deltaCost<true>(
                            delta,
                            Route::Proposal(route->before(i),
                                            PickupSegment(data, ship),
                                            DeliverySegment(data, ship),
                                            route->after(i + 1)));
                    else
                        costEvaluator.deltaCost<true>(
                            delta,
                            Route::Proposal(route->before(i),
                                            PickupSegment(data, ship),
                                            route->between(i + 1, j),
                                            DeliverySegment(data, ship),
                                            route->after(j + 1)));

                    if (delta < bestCost)
                    {
                        bestCost = delta;
                        bestRoute = route;
                        bestPick = i;
                        bestDeliv = j;
                    }
                }
        }

        if (!bestRoute)
            continue;

        bestRoute->insert(bestDeliv + 1, delivery);
        bestRoute->insert(bestPick + 1, pickup);
        bestRoute->update();

        assert(pickup->route() == delivery->route());
        assert(pickup->pos() < delivery->pos());

        searchSpace.markPromising(pickup);
        searchSpace.markPromising(delivery);
    }
}
