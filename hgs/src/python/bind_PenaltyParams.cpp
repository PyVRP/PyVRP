#include "PenaltyParams.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_PenaltyParams(py::module_ &m)
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
}
