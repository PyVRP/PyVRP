#include "RelocateStar.h"

void RelocateStar::init(Individual const &indiv)
{
    LocalSearchOperator<Route>::init(indiv);
    relocate.init(indiv);
}

TCost RelocateStar::evaluate(Route *U, Route *V)
{
    move = {};

    for (auto *nodeU = n(U->depot); !nodeU->isDepot(); nodeU = n(nodeU))
    {
        TCost deltaCost = relocate.evaluate(nodeU, V->depot);  // test after depot

        if (deltaCost < move.deltaCost)
            move = {deltaCost, nodeU, V->depot};

        for (auto *nodeV = n(V->depot); !nodeV->isDepot(); nodeV = n(nodeV))
        {
            deltaCost = relocate.evaluate(nodeU, nodeV);  // test U after V

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeU, nodeV};

            deltaCost = relocate.evaluate(nodeV, nodeU);  // test V after U

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeV, nodeU};
        }
    }

    return move.deltaCost;
}

void RelocateStar::apply(Route *U, Route *V)
{
    move.from->insertAfter(move.to);
}
