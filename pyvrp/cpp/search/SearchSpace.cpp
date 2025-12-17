#include "SearchSpace.h"

#include <stdexcept>

using pyvrp::search::SearchSpace;

SearchSpace::SearchSpace(ProblemData const &data, Neighbours neighbours)
    : data_(data),
      neighbours_(data.numLocations()),
      promising_(data.numLocations())
{
    setNeighbours(neighbours);
}

void SearchSpace::setNeighbours(Neighbours neighbours)
{
    if (neighbours.size() != data_.numLocations())
        throw std::runtime_error("Neighbourhood dimensions do not match.");

    for (size_t client = data_.numDepots(); client != data_.numLocations();
         ++client)
    {
        auto const beginPos = neighbours[client].begin();
        auto const endPos = neighbours[client].end();

        auto const pred = [&](auto item)
        { return item == client || item < data_.numDepots(); };

        if (std::any_of(beginPos, endPos, pred))
        {
            throw std::runtime_error("Neighbourhood of client "
                                     + std::to_string(client)
                                     + " contains itself or a depot.");
        }
    }

    neighbours_ = neighbours;
}

SearchSpace::Neighbours const &SearchSpace::neighbours() const
{
    return neighbours_;
}

std::vector<size_t> const &SearchSpace::neighboursOf(size_t client) const
{
    return neighbours_[client];
}

bool SearchSpace::isPromising(size_t client) const
{
    assert(client >= data_.numDepots());
    return promising_[client];
}

void SearchSpace::markPromising(size_t client)
{
    assert(client >= data_.numDepots());
    promising_[client] = true;
}

void SearchSpace::markAllPromising() { promising_.set(); }

void SearchSpace::unmarkAllPromising() { promising_.reset(); }
