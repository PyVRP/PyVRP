#include "PenaltyManager.h"

#include <pybind11/pybind11.h>

#include <algorithm>
#include <stdexcept>

namespace py = pybind11;

PenaltyManager::PenaltyManager(unsigned int vehicleCapacity,
                               PenaltyParams params)
    : params(params),
      vehicleCapacity(vehicleCapacity),
      capacityPenalty(params.initCapacityPenalty),
      timeWarpPenalty(params.initTimeWarpPenalty)
{
    if (params.penaltyIncrease < 1.)
        throw std::invalid_argument("Expected penaltyIncrease >= 1.");

    if (params.penaltyDecrease < 0. || params.penaltyDecrease > 1.)
        throw std::invalid_argument("Expected penaltyDecrease in [0, 1].");

    if (params.targetFeasible < 0. || params.targetFeasible > 1.)
        throw std::invalid_argument("Expected targetFeasible in [0, 1].");

    if (params.repairBooster < 1)
        throw std::invalid_argument("Expected repairBooster >= 1.");
}

unsigned int PenaltyManager::compute(unsigned int penalty, double feasPct) const
{
    auto const diff = params.targetFeasible - feasPct;

    if (-0.05 < diff && diff < 0.05)  // allow some margins on the difference
        return penalty;               // between target and actual

    auto dPenalty = static_cast<double>(penalty);

    // +- 1 to ensure we do not get stuck at the same integer values, bounded
    // to [1, 1000] to avoid overflow in cost computations.
    if (diff > 0)
        dPenalty = std::min(params.penaltyIncrease * dPenalty + 1, 1000.);
    else
        dPenalty = std::max(params.penaltyDecrease * dPenalty - 1, 1.);

    return static_cast<int>(dPenalty);
}

void PenaltyManager::updateCapacityPenalty(double currFeasPct)
{
    capacityPenalty = compute(capacityPenalty, currFeasPct);
}

void PenaltyManager::updateTimeWarpPenalty(double currFeasPct)
{
    timeWarpPenalty = compute(timeWarpPenalty, currFeasPct);
}

unsigned int PenaltyManager::loadPenalty(unsigned int load) const
{
    if (load > vehicleCapacity)
        return (load - vehicleCapacity) * capacityPenalty;

    return 0;
}

unsigned int PenaltyManager::twPenalty(unsigned int timeWarp) const
{
    return timeWarp * timeWarpPenalty;
}

PenaltyManager::PenaltyBooster PenaltyManager::getPenaltyBooster()
{
    return PenaltyBooster(*this);
}

PYBIND11_MODULE(PenaltyManager, m)
{
    py::class_<PenaltyParams>(m, "PenaltyParams")
        .def(py::init<unsigned int,
                      unsigned int,
                      unsigned int,
                      double,
                      double,
                      double>(),
             py::arg("init_capacity_penalty") = 20,
             py::arg("init_time_warp_penalty") = 6,
             py::arg("repair_booster") = 12,
             py::arg("penalty_increase") = 1.34,
             py::arg("penalty_decrease") = 0.32,
             py::arg("target_feasible") = 0.43)
        .def_readonly("init_capacity_penalty",
                      &PenaltyParams::initCapacityPenalty)
        .def_readonly("init_time_warp_penalty",
                      &PenaltyParams::initTimeWarpPenalty)
        .def_readonly("repair_booster", &PenaltyParams::repairBooster)
        .def_readonly("penalty_increase", &PenaltyParams::penaltyIncrease)
        .def_readonly("penalty_decrease", &PenaltyParams::penaltyDecrease)
        .def_readonly("target_feasible", &PenaltyParams::targetFeasible);

    py::class_<PenaltyManager::PenaltyBooster>(m, "PenaltyBooster")
        .def("__enter__",
             [](PenaltyManager::PenaltyBooster &booster) {
                 booster.enter();
                 return booster;
             })
        .def(
            "__exit__",
            [](PenaltyManager::PenaltyBooster &booster,
               py::object type,
               py::object value,
               py::object traceback) { booster.exit(); },
            py::arg("type"),
            py::arg("value"),
            py::arg("traceback"));

    py::class_<PenaltyManager>(m, "PenaltyManager")
        .def(py::init<unsigned int, PenaltyParams>(),
             py::arg("vehicle_capacity"),
             py::arg("params"))
        .def(py::init<unsigned int>(), py::arg("vehicle_capacity"))
        .def("update_capacity_penalty",
             &PenaltyManager::updateCapacityPenalty,
             py::arg("curr_feas_pct"))
        .def("update_time_warp_penalty",
             &PenaltyManager::updateTimeWarpPenalty,
             py::arg("curr_feas_pct"))
        .def("load_penalty", &PenaltyManager::loadPenalty, py::arg("load"))
        .def("tw_penalty", &PenaltyManager::twPenalty, py::arg("time_warp"))
        .def("get_penalty_booster", &PenaltyManager::getPenaltyBooster);
}
