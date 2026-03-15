#include "SearchSpace.h"

#include <cassert>
#include <numeric>
#include <stdexcept>

using pyvrp::search::Route;
using pyvrp::search::SearchSpace;

SearchSpace::SearchSpace(ProblemData const &data, Neighbours neighbours)
    : neighbours_(data.numClients()),
      promising_(data.numClients()),
      clientOrder_(data.numClients())
{
    setNeighbours(neighbours);
    std::iota(clientOrder_.begin(), clientOrder_.end(), 0);

    size_t offset = 0;
    for (size_t vehType = 0; vehType != data.numVehicleTypes(); vehType++)
    {
        vehTypeOrder_.emplace_back(vehType, offset);
        offset += data.vehicleType(vehType).numAvailable;
    }
}

void SearchSpace::setNeighbours(Neighbours neighbours)
{
    if (neighbours.size() != neighbours_.size())
        throw std::runtime_error("Neighbourhood dimensions do not match.");

    for (size_t client = 0; client != neighbours.size(); ++client)
    {
        auto const beginPos = neighbours[client].begin();
        auto const endPos = neighbours[client].end();

        auto const pred = [&](auto item) { return item == client; };

        if (std::any_of(beginPos, endPos, pred))
            throw std::runtime_error("Neighbourhood of client "
                                     + std::to_string(client)
                                     + " contains itself.");
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
    assert(client < neighbours_.size());
    return promising_[client];
}

void SearchSpace::markPromising(size_t client)
{
    assert(client < neighbours_.size());
    promising_[client] = true;
}

void SearchSpace::markPromising(Route::Node const *node)
{
    assert(node->route());

    if (node->isClient())
        markPromising(node->idx());

    if (!node->isStartDepot() && p(node)->isClient())
        markPromising(p(node)->idx());

    if (!node->isEndDepot() && n(node)->isClient())
        markPromising(n(node)->idx());
}

void SearchSpace::markAllPromising() { promising_.set(); }

void SearchSpace::unmarkAllPromising() { promising_.reset(); }

std::vector<size_t> const &SearchSpace::clientOrder() const
{
    return clientOrder_;
}

std::vector<std::pair<size_t, size_t>> const &SearchSpace::vehTypeOrder() const
{
    return vehTypeOrder_;
}

void SearchSpace::shuffle(RandomNumberGenerator &rng)
{
    rng.shuffle(clientOrder_.begin(), clientOrder_.end());
    rng.shuffle(vehTypeOrder_.begin(), vehTypeOrder_.end());
}
