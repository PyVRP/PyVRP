#include "diversity.h"

#include <limits>

std::vector<size_t>
pyvrp::diversity::differentNeighbours(pyvrp::Solution const &first,
                                      pyvrp::Solution const &second)
{
    auto const &fNeighbours = first.neighbours();
    auto const &sNeighbours = second.neighbours();

    size_t const numLocations = fNeighbours.size();
    std::vector<size_t> differences;

    for (size_t location = 0; location != numLocations; location++)
    {
        // Large default in case a location is not in one of the solutions.
        size_t constexpr max = std::numeric_limits<size_t>::max();
        std::pair<size_t, size_t> constexpr unassigned = {max, max};

        auto const [fPred, fSucc] = fNeighbours[location].value_or(unassigned);
        auto const [sPred, sSucc] = sNeighbours[location].value_or(unassigned);

        // An edge pair (fPred, location) or (location, fSucc) from the first
        // solution is broken if it is not in the second solution.
        if (fSucc != sSucc or fPred != sPred)
            differences.push_back(location);
    }

    return differences;
}
