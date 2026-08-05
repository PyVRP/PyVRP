#ifndef PYVRP_SEARCH_BINDINGS_H
#define PYVRP_SEARCH_BINDINGS_H

#include "neighbourhood.h"

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

namespace pybind11::detail
{
// Caster for neighbourhoods, which are represented as dictionaries on the
// Python side but use a custom class in C++.
template <> struct type_caster<pyvrp::search::Neighbourhood>
{
    PYBIND11_TYPE_CASTER(pyvrp::search::Neighbourhood,
                         _("dict[Activity, list[Activity]]"));

    bool load(pybind11::handle src,  // Python -> C++
              [[maybe_unused]] bool convert)
    {
        if (!pybind11::isinstance<pybind11::dict>(src))
            return false;

        auto const dict = pybind11::reinterpret_borrow<pybind11::dict>(src);

        size_t numClients = 0;
        size_t numShipments = 0;
        for (auto &[k, v] : dict)
        {
            auto const key = k.cast<pyvrp::Activity>();
            if (!key.isClient() && !key.isPickup())
            {
                auto const *msg = "Expected client and pickup activities!";
                throw pybind11::value_error(msg);
            }

            if (key.isClient())
                numClients++;
            else
                numShipments++;
        }

        std::vector<std::vector<pyvrp::Activity>> clients(numClients);
        std::vector<std::vector<pyvrp::Activity>> pickups(numShipments);

        for (auto &[k, v] : dict)
        {
            auto const key = k.cast<pyvrp::Activity>();
            auto const values = v.cast<std::vector<pyvrp::Activity>>();

            if (key.isClient())
            {
                if (key.idx() >= numClients)
                {
                    auto const *msg = "Activity index exceeds instance size.";
                    throw pybind11::value_error(msg);
                }

                clients[key.idx()] = values;
            }
            else
            {
                if (key.idx() >= numShipments)
                {
                    auto const *msg = "Activity index exceeds instance size.";
                    throw pybind11::value_error(msg);
                }

                pickups[key.idx()] = values;
            }
        }

        value = pyvrp::search::Neighbourhood(clients, pickups);
        return !PyErr_Occurred();
    }

    static pybind11::handle
    cast(pyvrp::search::Neighbourhood const &src,  // C++ -> Python
         pybind11::return_value_policy policy,
         pybind11::handle parent)
    {
        pybind11::dict d;

        auto const &clients = src.clients();
        for (size_t idx = 0; idx != clients.size(); ++idx)
        {
            pybind11::list lst;
            for (auto &a : clients[idx])
                lst.append(pybind11::cast(a, policy, parent));

            pyvrp::Activity act = {pyvrp::Activity::ActivityType::CLIENT, idx};
            pybind11::object key = pybind11::cast(act, policy, parent);
            d[key] = lst;
        }

        auto const &pickups = src.pickups();
        for (size_t idx = 0; idx != pickups.size(); ++idx)
        {
            pybind11::list lst;
            for (auto &a : pickups[idx])
                lst.append(pybind11::cast(a, policy, parent));

            pyvrp::Activity act = {pyvrp::Activity::ActivityType::PICKUP, idx};
            pybind11::object key = pybind11::cast(act, policy, parent);
            d[key] = lst;
        }

        return d.release();
    }
};
}  // namespace pybind11::detail

#endif  // PYVRP_SEARCH_BINDINGS_H
