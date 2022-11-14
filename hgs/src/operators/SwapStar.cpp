#include "SwapStar.h"

using TWS = TimeWindowSegment;

void SwapStar::updateRemovalCosts(Route *R1)
{
    auto const currTimeWarp = d_params.twPenalty(R1->timeWarp());

    for (Node *U = n(R1->depot); !U->isDepot(); U = n(U))
    {
        auto twData = TWS::merge(p(U)->twBefore, n(U)->twAfter);
        removalCosts(R1->idx, U->client)
            = d_params.dist(p(U)->client, n(U)->client)
              - d_params.dist(p(U)->client, U->client, n(U)->client)
              + d_params.twPenalty(twData.totalTimeWarp()) - currTimeWarp;
    }
}

void SwapStar::updateInsertionCost(Route *R, Node *U)
{
    auto &insertPositions = cache(R->idx, U->client);

    insertPositions = {};
    insertPositions.shouldUpdate = false;

    // Insert cost of U just after the depot (0 -> U -> ...)
    auto twData = TWS::merge(R->depot->twBefore, U->tw, n(R->depot)->twAfter);
    int cost = d_params.dist(0, U->client, n(R->depot)->client)
               - d_params.dist(0, n(R->depot)->client)
               + d_params.twPenalty(twData.totalTimeWarp())
               - d_params.twPenalty(R->timeWarp());

    insertPositions.maybeAdd(cost, R->depot);

    for (Node *V = n(R->depot); !V->isDepot(); V = n(V))
    {
        // Insert cost of U just after V (V -> U -> ...)
        twData = TWS::merge(V->twBefore, U->tw, n(V)->twAfter);
        int deltaCost = d_params.dist(V->client, U->client, n(V)->client)
                        - d_params.dist(V->client, n(V)->client)
                        + d_params.twPenalty(twData.totalTimeWarp())
                        - d_params.twPenalty(R->timeWarp());

        insertPositions.maybeAdd(deltaCost, V);
    }
}

std::pair<int, Node *> SwapStar::getBestInsertPoint(Node *U, Node *V)
{
    auto &best_ = cache(V->route->idx, U->client);

    if (best_.shouldUpdate)  // then we first update the insert positions
        updateInsertionCost(V->route, U);

    for (size_t idx = 0; idx != 3; ++idx)  // only OK if V is not adjacent
        if (best_.locs[idx] && best_.locs[idx] != V && n(best_.locs[idx]) != V)
            return std::make_pair(best_.costs[idx], best_.locs[idx]);

    // As a fallback option, we consider inserting in the place of V
    auto const twData = TWS::merge(p(V)->twBefore, U->tw, n(V)->twAfter);
    int deltaCost = d_params.dist(p(V)->client, U->client, n(V)->client)
                    - d_params.dist(p(V)->client, n(V)->client)
                    + d_params.twPenalty(twData.totalTimeWarp())
                    - d_params.twPenalty(V->route->timeWarp());

    return std::make_pair(deltaCost, p(V));
}

void SwapStar::init(Individual const &indiv)
{
    LocalSearchOperator<Route>::init(indiv);
    std::fill(updated.begin(), updated.end(), true);
}

int SwapStar::evaluate(Route *routeU, Route *routeV)
{
    best = {};

    if (updated[routeV->idx])
    {
        updateRemovalCosts(routeV);
        updated[routeV->idx] = false;

        for (int idx = 1; idx != d_params.nbClients + 1; ++idx)
            cache(routeV->idx, idx).shouldUpdate = true;
    }

    if (updated[routeU->idx])
    {
        updateRemovalCosts(routeU);
        updated[routeV->idx] = false;

        for (int idx = 1; idx != d_params.nbClients + 1; ++idx)
            cache(routeU->idx, idx).shouldUpdate = true;
    }

    for (Node *U = n(routeU->depot); !U->isDepot(); U = n(U))
        for (Node *V = n(routeV->depot); !V->isDepot(); V = n(V))
        {
            int deltaCost = 0;

            int const uDemand = d_params.clients[U->client].demand;
            int const vDemand = d_params.clients[V->client].demand;
            int const loadDiff = uDemand - vDemand;

            deltaCost += d_params.loadPenalty(routeU->load() - loadDiff);
            deltaCost -= d_params.loadPenalty(routeU->load());

            deltaCost += d_params.loadPenalty(routeV->load() + loadDiff);
            deltaCost -= d_params.loadPenalty(routeV->load());

            deltaCost += removalCosts(routeU->idx, U->client);
            deltaCost += removalCosts(routeV->idx, V->client);

            if (deltaCost >= 0)  // an early filter on many moves, before doing
                continue;        // costly work determining insertion points

            auto [extraV, UAfter] = getBestInsertPoint(U, V);
            deltaCost += extraV;

            if (deltaCost >= 0)  // continuing here avoids evaluating another
                continue;        // costly insertion point below

            auto [extraU, VAfter] = getBestInsertPoint(V, U);
            deltaCost += extraU;

            if (deltaCost < best.cost)
            {
                best.cost = deltaCost;

                best.U = U;
                best.UAfter = UAfter;

                best.V = V;
                best.VAfter = VAfter;
            }
        }

    // It is possible for positive delta costs to turn negative when we do a
    // complete evaluation. But in practice that almost never happens, and is
    // not worth spending time on.
    if (best.cost >= 0)
        return best.cost;

    // Now do a full evaluation of the proposed swap move. This includes
    // possible time warp penalties.
    int const current
        = d_params.dist(p(best.U)->client, best.U->client, n(best.U)->client)
          + d_params.dist(p(best.V)->client, best.V->client, n(best.V)->client);

    int const proposed = d_params.dist(best.VAfter->client, best.V->client)
                         + d_params.dist(best.UAfter->client, best.U->client);

    int deltaCost = proposed - current;

    if (best.VAfter == p(best.U))
    {
        // Insert in place of U
        deltaCost += d_params.dist(best.V->client, n(best.U)->client);
    }
    else
    {
        deltaCost
            += d_params.dist(best.V->client, n(best.VAfter)->client)
               + d_params.dist(p(best.U)->client, n(best.U)->client)
               - d_params.dist(best.VAfter->client, n(best.VAfter)->client);
    }

    if (best.UAfter == p(best.V))
    {
        // Insert in place of V
        deltaCost += d_params.dist(best.U->client, n(best.V)->client);
    }
    else
    {
        deltaCost
            += d_params.dist(best.U->client, n(best.UAfter)->client)
               + d_params.dist(p(best.V)->client, n(best.V)->client)
               - d_params.dist(best.UAfter->client, n(best.UAfter)->client);
    }

    // It is not possible to have UAfter == V or VAfter == U, so the positions
    // are always strictly different
    if (best.VAfter->position + 1 == best.U->position)
    {
        // Special case
        auto uTWS
            = TWS::merge(best.VAfter->twBefore, best.V->tw, n(best.U)->twAfter);

        deltaCost += d_params.twPenalty(uTWS.totalTimeWarp());
    }
    else if (best.VAfter->position < best.U->position)
    {
        auto uTWS = TWS::merge(
            best.VAfter->twBefore,
            best.V->tw,
            routeU->twBetween(best.VAfter->position + 1, best.U->position - 1),
            n(best.U)->twAfter);

        deltaCost += d_params.twPenalty(uTWS.totalTimeWarp());
    }
    else
    {
        auto uTWS = TWS::merge(
            p(best.U)->twBefore,
            routeU->twBetween(best.U->position + 1, best.VAfter->position),
            best.V->tw,
            n(best.VAfter)->twAfter);

        deltaCost += d_params.twPenalty(uTWS.totalTimeWarp());
    }

    if (best.UAfter->position + 1 == best.V->position)
    {
        // Special case
        auto vTWS
            = TWS::merge(best.UAfter->twBefore, best.U->tw, n(best.V)->twAfter);

        deltaCost += d_params.twPenalty(vTWS.totalTimeWarp());
    }
    else if (best.UAfter->position < best.V->position)
    {
        auto vTWS = TWS::merge(
            best.UAfter->twBefore,
            best.U->tw,
            routeV->twBetween(best.UAfter->position + 1, best.V->position - 1),
            n(best.V)->twAfter);

        deltaCost += d_params.twPenalty(vTWS.totalTimeWarp());
    }
    else
    {
        auto vTWS = TWS::merge(
            p(best.V)->twBefore,
            routeV->twBetween(best.V->position + 1, best.UAfter->position),
            best.U->tw,
            n(best.UAfter)->twAfter);

        deltaCost += d_params.twPenalty(vTWS.totalTimeWarp());
    }

    deltaCost -= d_params.twPenalty(routeU->timeWarp());
    deltaCost -= d_params.twPenalty(routeV->timeWarp());

    auto const uDemand = d_params.clients[best.U->client].demand;
    auto const vDemand = d_params.clients[best.V->client].demand;

    deltaCost += d_params.loadPenalty(routeU->load() - uDemand + vDemand);
    deltaCost -= d_params.loadPenalty(routeU->load());

    deltaCost += d_params.loadPenalty(routeV->load() + uDemand - vDemand);
    deltaCost -= d_params.loadPenalty(routeV->load());

    return deltaCost;
}

void SwapStar::apply(Route *U, Route *V)
{
    if (best.U && best.UAfter && best.V && best.VAfter)
    {
        best.U->insertAfter(best.UAfter);
        best.V->insertAfter(best.VAfter);
    }
}
