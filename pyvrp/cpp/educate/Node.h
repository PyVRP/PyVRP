#ifndef NODE_H
#define NODE_H

#include "ProblemData.h"
#include "TimeWindowSegment.h"
#include "precision.h"

class Route;

class Node
{
public:               // TODO make fields private
    int client;       // Client represented with this node
    size_t position;  // Position in the route
    Node *next;       // Next node in the route order
    Node *prev;       // Previous node in the route order
    Route *route;     // Pointer towards the associated route

    // TODO can these data fields be moved to Route?
    int cumLoad;            // Cumulative load of segment [0, .., client]
    distance_type cumDist;  // Cumulative dist of segment [0, .., client]
    distance_type cumDeltaRevDist;  // Delta cum dist if segment is reversed

    TimeWindowSegment tw;        // TWS for just this client
    TimeWindowSegment twBefore;  // TWS for [0, .., client - 1, client]
    TimeWindowSegment twAfter;   // TWS for [client, client + 1, .., 0]

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

#endif  // NODE_H
