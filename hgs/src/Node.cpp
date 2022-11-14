#include "Node.h"

void Node::insertAfter(Node *other)
{
    prev->next = next;
    next->prev = prev;
    other->next->prev = this;
    prev = other;
    next = other->next;
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
