#include "Node.h"

void Node::insertAfter(Node *other)
{
    if (route)  // If we're in a route, we first stitch up the current route.
    {           // If we're not in a route, this step should be skipped.
        prev->next = next;
        next->prev = prev;
    }

    prev = other;
    next = other->next;

    other->next->prev = this;
    other->next = this;

    route = other->route;
}

void Node::swapWith(Node *other)
{
    auto *VPred = other->prev;
    auto *VSucc = other->next;
    auto *UPred = prev;
    auto *USucc = next;

    auto *routeU = route;
    auto *routeV = other->route;

    UPred->next = other;
    USucc->prev = other;
    VPred->next = this;
    VSucc->prev = this;

    prev = VPred;
    next = VSucc;
    other->prev = UPred;
    other->next = USucc;

    route = routeV;
    other->route = routeU;
}

void Node::remove()
{
    prev->next = next;
    next->prev = prev;

    prev = nullptr;
    next = nullptr;
    route = nullptr;
}

Node* Node::clone() const 
{
    Node* clonedNode = new Node;

    clonedNode->client = this->client;
    clonedNode->position = this->position;
    clonedNode->route = nullptr;
    clonedNode->next = nullptr;
    clonedNode->prev = nullptr;
    
    clonedNode->cumulatedWeight = this->cumulatedWeight;
    clonedNode->cumulatedVolume = this->cumulatedVolume;
    clonedNode->cumulatedSalvage = this->cumulatedSalvage;
    clonedNode->cumulatedDistance = this->cumulatedDistance;
    clonedNode->cumulatedReversalDistance = this->cumulatedReversalDistance;
    
    clonedNode->tw = this->tw;
    clonedNode->twBefore = this->twBefore;
    clonedNode->twAfter = this->twAfter;
    
    return clonedNode;
}
