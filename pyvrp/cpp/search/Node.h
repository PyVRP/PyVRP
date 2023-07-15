#ifndef PYVRP_NODE_H
#define PYVRP_NODE_H

#include "Measure.h"
#include "TimeWindowSegment.h"

namespace pyvrp::search
{
class Route;

struct Node
{
    // TODO rename client to location/loc
    int client;       // Location represented by this node
    size_t position;  // Position in the route
    Node *next;       // Next node in the route order
    Node *prev;       // Previous node in the route order
    Route *route;     // Pointer towards the associated route

    // TODO can these data fields be moved to Route?
    Load cumulatedLoad = 0;                  // Load depot -> client (incl)
    Distance cumulatedDistance = 0;          // Dist depot -> client (incl)
    Distance cumulatedReversalDistance = 0;  // Dist if (0..client) is reversed

    TimeWindowSegment tw;        // TWS for individual node (client)
    TimeWindowSegment twBefore;  // TWS for (0...client) including self
    TimeWindowSegment twAfter;   // TWS for (client...0) including self

    [[nodiscard]] bool isDepot() const;

    /**
     * Inserts this node after the other and updates the relevant links.
     */
    void insertAfter(Node *other);

    /**
     * Swaps this node with the other and updates the relevant links.
     */
    void swapWith(Node *other);

    /**
     * Removes this node and updates the relevant links.
     */
    void remove();
};

/**
 * Convenience method accessing the node directly before the argument.
 */
inline Node *p(Node *node) { return node->prev; }

/**
 * Convenience method accessing the node directly after the argument.
 */
inline Node *n(Node *node) { return node->next; }
}  // namespace pyvrp::search

#endif  // PYVRP_NODE_H
