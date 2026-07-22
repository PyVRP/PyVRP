#include "RelocateShipment.h"

#include <cassert>

using pyvrp::search::RelocateShipment;

std::pair<pyvrp::Cost, bool> RelocateShipment::evaluate(
    Route::Node *U, Route::Node *V, CostEvaluator const &costEvaluator)
{
    assert(V->route());
    stats_.numEvaluations++;

    if (!U->isPickup() || !U->route() || !V->isShipment())
        return std::make_pair(0, false);

    Route::Node *uPickup = U;
    Route::Node *uDelivery = U + 1;
    Route::Node *vPickup, *vDelivery;
    if (V->isPickup())
    {
        vPickup = V;
        vDelivery = V + 1;
    }
    else
    {
        vPickup = V - 1;
        vDelivery = V;
    }

    if (U->route() == V->route())
    {
        if (uPickup->trip() != vPickup->trip())  // cannot evaluate relocating
            return std::make_pair(0, false);     // shipments over trips.

        auto const *route = U->route();

        if (uPickup->pos() < vPickup->pos())
        {
            // uPickup is first. Next is either uDelivery or vPickup.
            if (uDelivery->pos() < vPickup->pos())
            {
                Cost deltaCost = 0;
                if (n(uPickup) == uDelivery)
                {
                    auto const proposal = Route::Proposal(
                        route->before(uPickup->pos() - 1),
                        route->between(uDelivery->pos() + 1, vPickup->pos()),
                        route->at(uPickup->pos()),
                        route->between(vPickup->pos() + 1, vDelivery->pos()),
                        route->at(uDelivery->pos()),
                        route->after(vDelivery->pos() + 1));

                    costEvaluator.deltaCost(deltaCost, proposal);
                }
                else
                {
                    auto const proposal = Route::Proposal(
                        route->before(uPickup->pos() - 1),
                        route->between(uPickup->pos() + 1,
                                       uDelivery->pos() - 1),
                        route->between(uDelivery->pos() + 1, vPickup->pos()),
                        route->at(uPickup->pos()),
                        route->between(vPickup->pos() + 1, vDelivery->pos()),
                        route->at(uDelivery->pos()),
                        route->after(vDelivery->pos() + 1));

                    costEvaluator.deltaCost(deltaCost, proposal);
                }

                return std::make_pair(deltaCost, deltaCost < 0);
            }
            else
            {
                // Then vPickup is first. Next is either uDelivery or
                // vDelivery.
                if (uDelivery->pos() < vDelivery->pos())  // uDelivery
                {
                    Cost deltaCost = 0;
                    if (n(vPickup) == uDelivery)
                    {
                        auto const proposal = Route::Proposal(
                            route->before(uPickup->pos() - 1),
                            route->between(uPickup->pos() + 1, vPickup->pos()),
                            route->at(uPickup->pos()),
                            route->between(uDelivery->pos() + 1,
                                           vDelivery->pos()),
                            route->at(uDelivery->pos()),
                            route->after(vDelivery->pos() + 1));

                        costEvaluator.deltaCost(deltaCost, proposal);
                    }
                    else
                    {
                        auto const proposal = Route::Proposal(
                            route->before(uPickup->pos() - 1),
                            route->between(uPickup->pos() + 1, vPickup->pos()),
                            route->at(uPickup->pos()),
                            route->between(vPickup->pos() + 1,
                                           uDelivery->pos() - 1),
                            route->between(uDelivery->pos() + 1,
                                           vDelivery->pos()),
                            route->at(uDelivery->pos()),
                            route->after(vDelivery->pos() + 1));

                        costEvaluator.deltaCost(deltaCost, proposal);
                    }

                    return std::make_pair(deltaCost, deltaCost < 0);
                }
                else  // vPickup
                {
                    Cost deltaCost = 0;
                    if (n(vDelivery) == uDelivery)
                    {
                        auto const proposal = Route::Proposal(
                            route->before(uPickup->pos() - 1),
                            route->between(uPickup->pos() + 1, vPickup->pos()),
                            route->at(uPickup->pos()),
                            route->after(vPickup->pos() + 1));

                        costEvaluator.deltaCost(deltaCost, proposal);
                    }
                    else
                    {
                        auto const proposal = Route::Proposal(
                            route->before(uPickup->pos() - 1),
                            route->between(uPickup->pos() + 1, vPickup->pos()),
                            route->at(uPickup->pos()),
                            route->between(vPickup->pos() + 1,
                                           vDelivery->pos()),
                            route->at(uDelivery->pos()),
                            route->between(vDelivery->pos() + 1,
                                           uDelivery->pos() - 1),
                            route->after(uDelivery->pos() + 1));

                        costEvaluator.deltaCost(deltaCost, proposal);
                    }

                    return std::make_pair(deltaCost, deltaCost < 0);
                }
            }
        }
        else
        {
            // vPickup is first. Next is either uPickup or vDelivery.
            if (vDelivery->pos() < uPickup->pos())
            {
                Cost deltaCost = 0;
                if (n(vDelivery) == uPickup && n(uPickup) == uDelivery)
                {
                    auto const proposal = Route::Proposal(
                        route->before(vPickup->pos()),
                        route->at(uPickup->pos()),
                        route->between(vPickup->pos() + 1, vDelivery->pos()),
                        route->after(uDelivery->pos()));

                    costEvaluator.deltaCost(deltaCost, proposal);
                }
                else if (n(vDelivery) == uPickup)
                {
                    auto const proposal = Route::Proposal(
                        route->before(vPickup->pos()),
                        route->at(uPickup->pos()),
                        route->between(vPickup->pos() + 1, vDelivery->pos()),
                        route->at(uDelivery->pos()),
                        route->between(uPickup->pos() + 1,
                                       uDelivery->pos() - 1),
                        route->after(uDelivery->pos() + 1));

                    costEvaluator.deltaCost(deltaCost, proposal);
                }
                else if (n(uPickup) == uDelivery)
                {
                    auto const proposal = Route::Proposal(
                        route->before(vPickup->pos()),
                        route->at(uPickup->pos()),
                        route->between(vPickup->pos() + 1, vDelivery->pos()),
                        route->at(uDelivery->pos()),
                        route->between(vDelivery->pos() + 1,
                                       uPickup->pos() - 1),
                        route->after(uDelivery->pos() + 1));

                    costEvaluator.deltaCost(deltaCost, proposal);
                }
                else
                {
                    auto const proposal = Route::Proposal(
                        route->before(vPickup->pos()),
                        route->at(uPickup->pos()),
                        route->between(vPickup->pos() + 1, vDelivery->pos()),
                        route->at(uDelivery->pos()),
                        route->between(vDelivery->pos() + 1,
                                       uPickup->pos() - 1),
                        route->between(uPickup->pos() + 1,
                                       uDelivery->pos() - 1),
                        route->after(uDelivery->pos() + 1));

                    costEvaluator.deltaCost(deltaCost, proposal);
                }

                return std::make_pair(deltaCost, deltaCost < 0);
            }
            else
            {
                // Then uPickup is first. Next is either uDelivery or
                // vDelivery.
                if (uDelivery->pos() < vDelivery->pos())
                {
                    Cost deltaCost = 0;

                    if (n(vPickup) == uPickup)
                    {
                        auto const proposal = Route::Proposal(
                            route->before(uDelivery->pos() - 1),
                            route->between(uDelivery->pos() + 1,
                                           vDelivery->pos()),
                            route->at(uDelivery->pos()),
                            route->after(vDelivery->pos() + 1));

                        costEvaluator.deltaCost(deltaCost, proposal);
                    }
                    else if (n(uPickup) == uDelivery)
                    {
                        auto const proposal = Route::Proposal(
                            route->before(vPickup->pos()),
                            route->at(uPickup->pos()),
                            route->between(vPickup->pos() + 1,
                                           uPickup->pos() - 1),
                            route->between(uDelivery->pos() + 1,
                                           vDelivery->pos()),
                            route->at(uDelivery->pos()),
                            route->after(vDelivery->pos() + 1));

                        costEvaluator.deltaCost(deltaCost, proposal);
                    }
                    else
                    {
                        auto const proposal = Route::Proposal(
                            route->before(vPickup->pos()),
                            route->at(uPickup->pos()),
                            route->between(vPickup->pos() + 1,
                                           uPickup->pos() - 1),
                            route->between(uPickup->pos() + 1,
                                           uDelivery->pos() - 1),
                            route->between(uDelivery->pos() + 1,
                                           vDelivery->pos()),
                            route->at(uDelivery->pos()),
                            route->after(vDelivery->pos() + 1));

                        costEvaluator.deltaCost(deltaCost, proposal);
                    }

                    return std::make_pair(deltaCost, deltaCost < 0);
                }
                else
                {
                    Cost deltaCost = 0;

                    if (n(vPickup) == uPickup && n(vDelivery) == uDelivery)
                        return std::make_pair(0, false);  // no-op
                    else if (n(vPickup) == uPickup)
                    {
                        auto const proposal = Route::Proposal(
                            route->before(vDelivery->pos()),
                            route->at(uDelivery->pos()),
                            route->between(vDelivery->pos() + 1,
                                           uDelivery->pos() - 1),
                            route->after(uDelivery->pos() + 1));

                        costEvaluator.deltaCost(deltaCost, proposal);
                    }
                    else if (n(vDelivery) == uDelivery)
                    {
                        auto const proposal = Route::Proposal(
                            route->before(vPickup->pos()),
                            route->at(uPickup->pos()),
                            route->between(vPickup->pos() + 1,
                                           uPickup->pos() - 1),
                            route->after(uPickup->pos() + 1));

                        costEvaluator.deltaCost(deltaCost, proposal);
                    }
                    else
                    {
                        auto const proposal = Route::Proposal(
                            route->before(vPickup->pos()),
                            route->at(uPickup->pos()),
                            route->between(vPickup->pos() + 1,
                                           uPickup->pos() - 1),
                            route->between(uPickup->pos() + 1,
                                           vDelivery->pos()),
                            route->at(uDelivery->pos()),
                            route->between(vDelivery->pos() + 1,
                                           uDelivery->pos() - 1),
                            route->after(uDelivery->pos() + 1));

                        costEvaluator.deltaCost(deltaCost, proposal);
                    }

                    return std::make_pair(deltaCost, deltaCost < 0);
                }
            }
        }
    }

    auto const *uRoute = U->route();
    auto const *vRoute = V->route();

    Cost deltaCost = 0;
    auto const vProposal
        = Route::Proposal(vRoute->before(vPickup->pos()),
                          uRoute->at(uPickup->pos()),
                          vRoute->between(vPickup->pos() + 1, vDelivery->pos()),
                          uRoute->at(uDelivery->pos()),
                          vRoute->after(vDelivery->pos() + 1));

    if (n(uPickup) != uDelivery)
    {
        auto const uProposal = Route::Proposal(
            uRoute->before(uPickup->pos() - 1),
            uRoute->between(uPickup->pos() + 1, uDelivery->pos() - 1),
            uRoute->after(uDelivery->pos() + 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }
    else
    {
        auto const uProposal
            = Route::Proposal(uRoute->before(uPickup->pos() - 1),
                              uRoute->after(uDelivery->pos() + 1));

        costEvaluator.deltaCost(deltaCost, uProposal, vProposal);
    }

    return std::make_pair(deltaCost, deltaCost < 0);
}

void RelocateShipment::apply(Route::Node *U, Route::Node *V) const
{
    assert(U->isPickup() && U->route() && V->isShipment() && V->route());
    stats_.numApplications++;

    Route::Node *vPickup, *vDelivery;
    if (V->isPickup())
    {
        vPickup = V;
        vDelivery = V + 1;
    }
    else
    {
        vPickup = V - 1;
        vDelivery = V;
    }

    auto *uRoute = U->route();
    uRoute->remove((U + 1)->pos());  // remove delivery
    uRoute->remove(U->pos());        // remove pickup

    auto *vRoute = V->route();
    vRoute->insert(vDelivery->pos() + 1, U + 1);  // insert delivery
    vRoute->insert(vPickup->pos() + 1, U);        // insert pickup
}

std::string RelocateShipment::name() const { return "RelocateShipment"; }

bool RelocateShipment::supports(ProblemData const &data)
{
    return data.numShipments() > 1;  // needs multiple shipments
}
