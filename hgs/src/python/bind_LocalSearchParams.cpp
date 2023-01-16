#include "LocalSearchParams.h"
#include "bindings.h"

namespace py = pybind11;

void bind_LocalSearchParams(py::module_ &m)
{
    py::class_<LocalSearchParams>(m, "LocalSearchParams")
        .def(py::init<size_t, size_t, size_t, size_t>(),
             py::arg("weight_wait_time") = 18,
             py::arg("weight_time_warp") = 20,
             py::arg("nb_granular") = 34,
             py::arg("post_process_path_length") = 7)
        .def_readonly("weight_wait_time", &LocalSearchParams::weightWaitTime)
        .def_readonly("weight_time_warp", &LocalSearchParams::weightTimeWarp)
        .def_readonly("nb_granular", &LocalSearchParams::nbGranular)
        .def_readonly("post_process_path_length",
                      &LocalSearchParams::postProcessPathLength);
}
