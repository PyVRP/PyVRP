#include "PenaltyManager.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(PenaltyManager, m)
{
    py::class_<PenaltyParams>(m, "PenaltyParams")
        .def(py::init<unsigned int,
                      unsigned int,
                      unsigned int,
                      unsigned int,
                      double,
                      double,
                      double>(),
             py::arg("init_capacity_penalty") = 20,
             py::arg("init_time_warp_penalty") = 6,
             py::arg("repair_booster") = 12,
             py::arg("num_registrations_between_penalty_updates") = 50,
             py::arg("penalty_increase") = 1.34,
             py::arg("penalty_decrease") = 0.32,
             py::arg("target_feasible") = 0.43)
        .def_readonly("init_capacity_penalty",
                      &PenaltyParams::initCapacityPenalty)
        .def_readonly("init_time_warp_penalty",
                      &PenaltyParams::initTimeWarpPenalty)
        .def_readonly("repair_booster", &PenaltyParams::repairBooster)
        .def_readonly("num_registrations_between_penalty_updates", 
                      &PenaltyParams::numRegistrationsBetweenPenaltyUpdates)
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
        .def("register_load_feasible",
             &PenaltyManager::registerLoadFeasible,
             py::arg("is_load_feasible"))
        .def("register_time_feasible",
             &PenaltyManager::registerTimeFeasible,
             py::arg("is_time_feasible"))
        .def("load_penalty", &PenaltyManager::loadPenalty, py::arg("load"))
        .def("tw_penalty", &PenaltyManager::twPenalty, py::arg("time_warp"))
        .def("get_penalty_booster", &PenaltyManager::getPenaltyBooster);
}
