#include "PenaltyManager.h"
#include "bindings.h"

namespace py = pybind11;

void bind_PenaltyManager(py::module_ &m)
{
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
        .def("update_capacity_penalty", &PenaltyManager::updateCapacityPenalty)
        .def("update_time_warp_penalty", &PenaltyManager::updateTimeWarpPenalty)
        .def("load_penalty", &PenaltyManager::loadPenalty)
        .def("tw_penalty", &PenaltyManager::twPenalty)
        .def("get_penalty_booster", &PenaltyManager::getPenaltyBooster);
}
