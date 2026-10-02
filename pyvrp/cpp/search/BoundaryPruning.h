#ifndef PYVRP_SEARCH_BOUNDARYPRUNING_H
#define PYVRP_SEARCH_BOUNDARYPRUNING_H
#include "Route.h"
#include <cfenv>
#include <cmath>
#include <limits>
namespace pyvrp::search
{
// Restricted-model early rejection; see dev/boundary_pruning.rst.
// No shared mutable state: support and magnitude belong to this operator.
class BoundaryPruning
{
    ProblemData const &data_;
    bool enabled_ = false;
    double maxEdge_ = 0;
    bool reject(double delta,
                Route const *u,
                Route const *v,
                CostEvaluator const &evaluator) const
    {
        const double cu = evaluator.penalisedCost(*u).get(),
                     cv = evaluator.penalisedCost(*v).get();
        const double du = u->distance().get(), dv = v->distance().get(),
                     maxEdge = maxEdge_;
        if (!(std::isfinite(delta) && std::isfinite(cu) && std::isfinite(cv)
              && std::isfinite(du) && std::isfinite(dv)
              && std::isfinite(maxEdge))
            || cu < 0 || cv < 0 || du < 0 || dv < 0 || maxEdge < 0)
            return false;
        const double penalty = (cu - du) + (cv - dv);
        const double margin
            = 1e-6 + (1 + maxEdge) * 0x1p-24 + (1 + cu + cv) * 0x1p-40;
        const double threshold = penalty + margin;
        return penalty >= 0 && std::isfinite(threshold) && delta > threshold;
    }

public:
    explicit BoundaryPruning(ProblemData const &data, bool enable = true)
        : data_(data)
    {
        static_assert(std::numeric_limits<double>::is_iec559
                      && std::numeric_limits<double>::digits == 53);
        if (!enable || std::fegetround() != FE_TONEAREST)
            return;
        if (data.numClients() > 1000 || data.numLocations() > 1000
            || data.numShipments() || data.numDepots() != 1
            || data.numVehicleTypes() != 1 || data.numProfiles() != 1
            || data.numGroups())
            return;
        const auto &vehicle = data.vehicleType(0);
        if (vehicle.fixedCost.get() != 0 || vehicle.unitDistanceCost.get() != 1
            || vehicle.unitDurationCost.get() != 0
            || vehicle.unitOvertimeCost.get() != 0
            || !vehicle.reloadDepots.empty()
            || vehicle.startDepot != vehicle.endDepot)
            return;
        for (std::size_t i = 0; i < data.numClients(); ++i)
        {
            const auto &client = data.client(i);
            if (!client.required || client.group || client.prize.get() != 0)
                return;
            for (auto pickup : client.pickup)
                if (pickup.get() != 0)
                    return;
        }
        for (std::size_t i = 0; i < data.numLocations(); ++i)
            if (data.distanceMatrix(0)(i, i).get() != 0)
                return;
        for (std::size_t i = 0; i < data.numLocations(); ++i)
            for (std::size_t j = 0; j < data.numLocations(); ++j)
            {
                const auto edge = data.distanceMatrix(0)(i, j).get();
                if (!std::isfinite(edge) || edge < 0 || edge > 1e12)
                    return;
                maxEdge_ = std::max(maxEdge_, edge);
            }
        enabled_ = true;
    }
    template <std::size_t N, std::size_t M>
    bool
    block(Route::Node *U, Route::Node *V, const CostEvaluator &evaluator) const
    {
        if (!enabled_)
            return false;
        auto const &data = data_;
        static_assert(N >= 1 && N <= 2 && M <= N);
        auto *rU = U->route();
        auto *rV = V->route();
        auto loc = [](auto *route, std::size_t pos)
        { return route->at(pos).front().location(); };
        const auto i = U->pos(), j = V->pos();
        const auto a = loc(rU, i - 1), u = loc(rU, i), x = loc(rU, i + N - 1),
                   b = loc(rU, i + N);
        const auto &matrix = data.distanceMatrix(0);
        auto d = [&](std::size_t p, std::size_t q) -> double
        { return matrix(p, q).get(); };
        double delta;
        if constexpr (M == 0)
            delta = d(a, b) - d(a, u) - d(x, b) + d(loc(rV, j), u)
                    + d(x, loc(rV, j + 1)) - d(loc(rV, j), loc(rV, j + 1));
        else
            delta = d(a, loc(rV, j)) + d(loc(rV, j + M - 1), b)
                    + d(loc(rV, j - 1), u) + d(x, loc(rV, j + M)) - d(a, u)
                    - d(x, b) - d(loc(rV, j - 1), loc(rV, j))
                    - d(loc(rV, j + M - 1), loc(rV, j + M));
        return reject(delta, rU, rV, evaluator);
    }
    bool
    tail(Route::Node *U, Route::Node *V, const CostEvaluator &evaluator) const
    {
        if (!enabled_)
            return false;
        auto const &data = data_;
        auto *rU = U->route();
        auto *rV = V->route();
        auto loc = [](auto *route, std::size_t pos)
        { return route->at(pos).front().location(); };
        const auto &matrix = data.distanceMatrix(0);
        auto d = [&](std::size_t p, std::size_t q) -> double
        { return matrix(p, q).get(); };
        const auto u = loc(rU, U->pos()), x = loc(rU, U->pos() + 1),
                   v = loc(rV, V->pos()), w = loc(rV, V->pos() + 1);
        const auto delta = d(u, w) + d(v, x) - d(u, x) - d(v, w);
        return reject(delta, rU, rV, evaluator);
    }
};
}  // namespace pyvrp::search
#endif  // PYVRP_SEARCH_BOUNDARYPRUNING_H
