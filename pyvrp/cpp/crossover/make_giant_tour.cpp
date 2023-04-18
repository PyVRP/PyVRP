#include "make_giant_tour.h"

void makeGiantTour(Individual &individual, ProblemData const *data)
{
    auto routes = individual.routes;

    // a precedes b only when a is not empty and b is. Combined with a stable
    // sort, this ensures we keep the original sorting as much as possible, but
    // also make sure all empty routes are at the end of routes_.
    auto comp = [](auto &a, auto &b) { return !a.empty() && b.empty(); };
    std::stable_sort(routes.begin(), routes.end(), comp);

    std::vector<int> tour;
    tour.reserve(data->nbClients);

    for (auto const &route : routes)
        for (Client c : route)
            tour.push_back(c);

    return tour;
};
