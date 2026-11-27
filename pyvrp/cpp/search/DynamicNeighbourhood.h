#ifndef PYVRP_SEARCH_DYNAMICNEIGHBOURHOOD_H
#define PYVRP_SEARCH_DYNAMICNEIGHBOURHOOD_H

#include "Activity.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pyvrp::search
{
/**
 * Ordered neighbourhoods that widen from quarter to half to full size after
 * repeated unsuccessful participations. Pickup and delivery share one owner.
 */
class DynamicNeighbourhood
{
public:
    using Neighbours = std::unordered_map<Activity, std::vector<Activity>>;
    static constexpr size_t DEFAULT_PATIENCE = 1000;

private:
    struct Row
    {
        std::vector<Activity> full, active;
        size_t divisor = 4;
        size_t failures = 0;

        size_t limit() const
        {
            auto const size = full.size();
            return std::min(
                size, std::max<size_t>(1, (size + divisor / 2) / divisor));
        }
    };

    size_t const patience_;
    std::unordered_map<Activity, Row> rows_;
    std::vector<Activity> cache_;

    static Activity owner(Activity const &activity)
    {
        return activity.isDelivery()
                   ? Activity(Activity::ActivityType::PICKUP, activity.idx())
                   : activity;
    }

    bool selects(Activity const &from, Activity const &to) const
    {
        auto const &row = rows_.at(owner(from));
        return std::any_of(row.full.begin(),
                           row.full.begin() + row.limit(),
                           [&](auto const &other)
                           { return owner(other) == to; });
    }

    void rebuild(Activity const &activity)
    {
        auto &row = rows_.at(activity);
        row.active.clear();
        for (size_t pos = 0; pos != row.full.size(); ++pos)
            if (pos < row.limit() || selects(row.full[pos], activity))
                row.active.push_back(row.full[pos]);
    }

public:
    DynamicNeighbourhood(Neighbours const &neighbours,
                         size_t clients,
                         size_t shipments,
                         size_t patience = DEFAULT_PATIENCE)
        : patience_(patience)
    {
        if (!patience || neighbours.size() != clients + shipments)
            throw std::invalid_argument(
                "Invalid dynamic neighbourhood size or patience.");

        auto const valid = [&](Activity const &activity)
        {
            return (activity.isClient() && activity.idx() < clients)
                   || (activity.isShipment() && activity.idx() < shipments);
        };
        for (auto const &[activity, full] : neighbours)
        {
            if (!valid(activity) || activity.isDelivery())
                throw std::invalid_argument("Invalid client or pickup key.");
            for (auto const &other : full)
                if (!valid(other) || owner(other) == activity)
                    throw std::invalid_argument("Invalid neighbour activity.");
            rows_[activity].full = full;
        }

        for (auto const &[activity, row] : rows_)
            rebuild(activity);
        cache_.reserve(51);
    }

    std::vector<Activity> const &neighboursOf(Activity const &activity) const
    {
        return rows_.at(owner(activity)).active;
    }

    Neighbours neighbours() const
    {
        Neighbours result;
        for (auto const &[activity, row] : rows_)
            result.emplace(activity, row.active);
        return result;
    }

    void touch(Activity const &activity)
    {
        if (activity.isDepot())
            return;

        auto const key = owner(activity);
        if (!rows_.contains(key))
            throw std::out_of_range("Invalid activity.");
        std::erase(cache_, key);
        cache_.insert(cache_.begin(), key);
        if (cache_.size() > 50)
            cache_.pop_back();
    }

    /** Update once per outer ILS iteration, after local search finishes. */
    void update(bool newBest)
    {
        std::unordered_set<Activity> changed;
        for (auto const &activity : cache_)
        {
            auto &row = rows_.at(activity);
            if (newBest || ++row.failures >= patience_)
            {
                row.failures = 0;
                auto const oldSize = row.limit();
                row.divisor
                    = newBest ? 4 : std::max<size_t>(1, row.divisor / 2);
                auto const newSize = row.limit();
                if (oldSize == newSize)
                    continue;

                changed.insert(activity);
                for (size_t pos = std::min(oldSize, newSize);
                     pos != std::max(oldSize, newSize);
                     ++pos)
                    changed.insert(owner(row.full[pos]));
            }
        }
        for (auto const &activity : changed)
            rebuild(activity);
        cache_.clear();
    }
};
}  // namespace pyvrp::search

#endif  // PYVRP_SEARCH_DYNAMICNEIGHBOURHOOD_H
