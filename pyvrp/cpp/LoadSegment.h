#ifndef PYVRP_LOADSEGMENT_H
#define PYVRP_LOADSEGMENT_H

#include "Measure.h"
#include "ProblemData.h"

#include <cassert>

namespace pyvrp
{
/**
 * LoadSegment(delivery: list[int], pickup: list[int], load: list[int])
 *
 * Creates a new load segment. Load segments can be efficiently concatenated,
 * and track statistics about capacity violations resulting from visiting
 * clients in the concatenated order.
 *
 * Parameters
 * ----------
 * delivery
 *     Total delivery amounts on this segment.
 * pickup
 *     Total pickup amounts on this segment.
 * load
 *     Maximum load amounts on this segment.
 */
class LoadSegment
{
    std::vector<Load> delivery_;
    std::vector<Load> pickup_;
    std::vector<Load> load_;

public:
    template <typename... Args>
    [[nodiscard]] static LoadSegment
    merge(LoadSegment const &first, LoadSegment const &second, Args &&...args);

    /**
     * Returns the amounts delivered to clients on this segment.
     */
    [[nodiscard]] std::vector<Load> delivery() const;

    /**
     * Returns the amounts picked up from clients on this segment.
     */
    [[nodiscard]] std::vector<Load> pickup() const;

    /**
     * Returns the maximum load amounts encountered on this segment.
     */
    [[nodiscard]] inline std::vector<Load> load() const;

    // Construct from attributes of the given client.
    LoadSegment(ProblemData::Client const &client);

    // Construct from raw data.
    inline LoadSegment(std::vector<Load> delivery,
                       std::vector<Load> pickup,
                       std::vector<Load> load);

    // Move or copy construct from the other load segment.
    inline LoadSegment(LoadSegment const &) = default;
    inline LoadSegment(LoadSegment &&) = default;

    // Move or copy assign form the other load segment.
    inline LoadSegment &operator=(LoadSegment const &) = default;
    inline LoadSegment &operator=(LoadSegment &&) = default;
};

template <typename... Args>
LoadSegment LoadSegment::merge(LoadSegment const &first,
                               LoadSegment const &second,
                               Args &&...args)
{
    assert(first.delivery_.size() == second.delivery_.size());
    assert(first.pickup_.size() == second.pickup_.size());
    assert(first.load_.size() == second.load_.size());

    // See Vidal et al. (2014) for details. This function implements equations
    // (9) -- (11) of https://doi.org/10.1016/j.ejor.2013.09.045 for each load
    // dimension.
    std::vector<Load> delivery(first.delivery_.size());
    std::vector<Load> pickup(first.pickup_.size());
    std::vector<Load> load(first.load_.size());

    for (size_t idx = 0; idx != first.delivery_.size(); ++idx)
    {
        delivery[idx] = first.delivery_[idx] + second.delivery_[idx];
        pickup[idx] = first.pickup_[idx] + second.pickup_[idx];
        load[idx] = std::max(first.load_[idx] + second.delivery_[idx],
                             second.load_[idx] + first.pickup_[idx]);
    }

    if constexpr (sizeof...(args) == 0)
        return {delivery, pickup, load};
    else
        return merge({delivery, pickup, load}, args...);
}

std::vector<Load> LoadSegment::load() const { return load_; }

LoadSegment::LoadSegment(std::vector<Load> delivery,
                         std::vector<Load> pickup,
                         std::vector<Load> load)
    : delivery_(std::move(delivery)),
      pickup_(std::move(pickup)),
      load_(std::move(load))
{
    assert(delivery_.size() == pickup_.size());
    assert(delivery_.size() == load_.size());
}
}  // namespace pyvrp

#endif  // PYVRP_LOADSEGMENT_H
