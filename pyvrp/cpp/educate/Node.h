#ifndef NODE_H
#define NODE_H

#include "ProblemData.h"
#include "TimeWindowSegment.h"

#ifdef INT_PRECISION
using TCost = int;
using TDist = int;
using TTime = int;
#else
using TCost = double;
using TDist = double;
using TTime = double;
#endif

class Route;

class Node
{
public:  // TODO make fields private
    ProblemData const *data;

    int client;       // Client represented with this node
    size_t position;  // Position in the route
    Node *next;       // Next node in the route order
    Node *prev;       // Previous node in the route order
    Route *route;     // Pointer towards the associated route

    int cumulatedLoad;              // Load from depot to client (inclusive)
    TDist cumulatedDistance;          // Distance from depot to client (inclusive)
    TDist cumulatedReversalDistance;  // Distance if (0 .. client) is reversed

    TimeWindowSegment tw;        // TWS for individual node (client)
    TimeWindowSegment twBefore;  // TWS for (0...client) including self
    TimeWindowSegment twAfter;   // TWS for (client...0) including self

    [[nodiscard]] inline bool isDepot() const;

    /**
     * Inserts this node after the other and updates the solution.
     */
    void insertAfter(Node *other);

    /**
     * Swaps this node with the other and updates the solution.
     */
    void swapWith(Node *other);
};

bool Node::isDepot() const { return client == 0; }

/**
 * Convenience method accessing the node directly before the argument.
 */
inline Node *p(Node *node) { return node->prev; }

/**
 * Convenience method accessing the node directly after the argument.
 */
inline Node *n(Node *node) { return node->next; }

/**
 * Convenience method accessing the node two positions after the argument.
 */
inline Node *nn(Node *node) { return node->next->next; }

#endif  // NODE_H
