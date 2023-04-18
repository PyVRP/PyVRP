#include "split_giant_tour.h"

namespace
{
// Structure representing a client when making routes
struct ClientSplit
{
    int demand = 0;       // The demand of the client
    int serviceTime = 0;  // The service duration of the client
    int d0_x = 0;         // The distance from the depot to the client
    int dx_0 = 0;         // The distance from the client to the depot
    int dnext = 0;        // The distance from the client to the next client
};

struct ClientSplits
{
    ProblemData const &data;

    std::vector<ClientSplit> splits;
    std::vector<int> predecessors;
    std::vector<int> pathCosts;

    std::vector<int> cumDist;
    std::vector<int> cumLoad;
    std::vector<int> cumServ;

    ClientSplits(ProblemData const &data, std::vector<int> const &tour)
        : data(data),
          splits(data.nbClients + 1),
          predecessors(data.nbClients + 1, 0),
          pathCosts(data.nbClients + 1, INT_MAX),
          cumDist(data.nbClients + 1, 0),
          cumLoad(data.nbClients + 1, 0),
          cumServ(data.nbClients + 1, 0)
    {
        pathCosts[0] = 0;

        for (int idx = 1; idx <= data.nbClients; idx++)  // exclude depot
        {
            auto const curr = tour[idx - 1];
            auto const dist = idx < data.nbClients  // INT_MIN for last client
                                  ? data.dist(curr, tour[idx])
                                  : INT_MIN;

            splits[idx] = {data.clients[curr].demand,
                           data.clients[curr].servDur,
                           data.dist(0, curr),
                           data.dist(curr, 0),
                           dist};

            cumLoad[idx] = cumLoad[idx - 1] + splits[idx].demand;
            cumServ[idx] = cumServ[idx - 1] + splits[idx].serviceTime;
            cumDist[idx] = cumDist[idx - 1] + splits[idx - 1].dnext;
        }
    }

    // Computes the cost of propagating label i to j
    [[nodiscard]] int propagate(int i, int j) const
    {
        assert(i < j);
        auto const deltaDist = cumDist[j] - cumDist[i + 1];
        return pathCosts[i] + deltaDist + splits[i + 1].d0_x + splits[j].dx_0
               + data.loadPenalty(cumLoad[j] - cumLoad[i]);
    }

    // Tests if i dominates j as a predecessor for all nodes x >= j + 1
    [[nodiscard]] bool leftDominates(int i, int j) const
    {
        assert(i < j);
        auto const lhs = pathCosts[j] + splits[j + 1].d0_x;
        auto const deltaDist = cumDist[j] - cumDist[i + 1];
        auto const rhs = pathCosts[i] + splits[i + 1].d0_x + deltaDist
                         + data.penaltyCapacity * (cumLoad[j] - cumLoad[i]);

        return lhs >= rhs;
    }

    // Tests if j dominates i as a predecessor for all nodes x >= j + 1
    [[nodiscard]] bool rightDominates(int i, int j) const
    {
        assert(i < j);
        auto const lhs = pathCosts[j] + splits[j + 1].d0_x;
        auto const rhs = pathCosts[i] + splits[i + 1].d0_x + cumDist[j + 1]
                         - cumDist[i + 1];

        return lhs <= rhs;
    };
};
}  // namespace

void makeRoutes()
{
    ClientSplits splits(*data, getTour());

    auto deq = std::deque<int>(data->nbClients + 1);
    deq.push_front(0);  // depot

    for (int idx = 1; idx <= data->nbClients; idx++)  // exclude depot
    {
        splits.pathCosts[idx] = splits.propagate(deq.front(), idx);
        splits.predecessors[idx] = deq.front();  // best predecessor for idx

        if (idx == data->nbClients)
            break;

        // idx will be inserted if idx is not dominated by the last client:
        // we need to remove whoever is dominated by idx.
        if (!splits.leftDominates(deq.back(), idx))
        {
            while (!deq.empty() && splits.rightDominates(deq.back(), idx))
                deq.pop_back();

            deq.push_back(idx);
        }

        while (deq.size() >= 2)  // Check if the current front is dominated by
        {                        // the follow-up client. If so, remove.
            auto const firstProp = splits.propagate(deq[0], idx + 1);
            auto const secondProp = splits.propagate(deq[1], idx + 1);

            if (firstProp >= secondProp)
                deq.pop_front();
            else
                break;
        }
    }

    if (splits.pathCosts[data->nbClients] == INT_MAX)  // has not been updated
        throw std::runtime_error("No split solution reached the last client");

    int end = data->nbClients;
    for (auto &route : routes_)
    {
        if (end == 0)
            break;

        int begin = splits.predecessors[end];
        route = {tour_.begin() + begin, tour_.begin() + end};
        end = begin;
    }

    assert(end == 0);
    evaluateCompleteCost();
    return routes;  // TODO
}
