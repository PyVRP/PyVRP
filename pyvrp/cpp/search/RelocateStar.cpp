#include "RelocateStar.h"

Cost RelocateStar::evaluate(Route *U, Route *V, CostEvaluator const &costEvaluator)
{
    move = {};

    for (auto *nodeU = n(U->depot); !nodeU->isDepot(); nodeU = n(nodeU))
    {   
        if (checkSalvageSequenceConstraint(nodeU, V->depot))
            continue;

        // Test inserting U after V's depot
        Cost deltaCost = relocate.evaluate(nodeU, V->depot, costEvaluator);
        
        if (deltaCost < move.deltaCost)
            move = {deltaCost, nodeU, V->depot};
        
        for (auto *nodeV = n(V->depot); !nodeV->isDepot(); nodeV = n(nodeV))
        {   
            if (checkSalvageSequenceConstraint(nodeU, nodeV))
                continue;

            // Test inserting U after V
            deltaCost = relocate.evaluate(nodeU, nodeV, costEvaluator);

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeU, nodeV};
            
            if (checkSalvageSequenceConstraint(nodeV, nodeU))
                continue;

            // Test inserting V after U
            deltaCost = relocate.evaluate(nodeV, nodeU, costEvaluator);

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeV, nodeU};
        }
    }

    return move.deltaCost;
}

bool RelocateStar::checkSalvageSequenceConstraint(Node *U, Node *V) const
{
    // These sequences should violate the constraint
    // S-B
    // S-D
    // B-B
    // B-D
    bool uIsClientDelivery = (data.client(U->client).demandWeight || data.client(U->client).demandVolume);
    bool uIsClientSalvage = (data.client(U->client).demandSalvage != Measure<MeasureType::SALVAGE>(0));
    bool uIsBoth = uIsClientDelivery && uIsClientSalvage;
    
    bool vIsClientDelivery = (data.client(V->client).demandWeight || data.client(V->client).demandVolume);
    bool vIsClientSalvage = (data.client(V->client).demandSalvage != Measure<MeasureType::SALVAGE>(0));
    bool vIsBoth = vIsClientDelivery && vIsClientSalvage;
    
    bool nextUClientDelivery = (data.client(n(U)->client).demandWeight || data.client(n(U)->client).demandVolume);
    bool nextVClientDelivery = (data.client(n(V)->client).demandWeight || data.client(n(V)->client).demandVolume);
    
    // S-B or S-D
    if (uIsClientSalvage && !uIsBoth && ((vIsClientDelivery || vIsBoth) || nextVClientDelivery))
        return true;

    // B-B or B-D
    if (uIsBoth && ((vIsBoth || vIsClientDelivery) || nextUClientDelivery))
        return true;

    return false;
}

void RelocateStar::apply([[maybe_unused]] Route *U,
                         [[maybe_unused]] Route *V) const
{
    move.from->insertAfter(move.to);
}
