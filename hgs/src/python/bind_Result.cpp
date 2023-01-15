#include "Result.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void bind_Result(py::module_ &m)
{
    py::class_<Result>(m, "Result")
        .def("get_best_found",
             &Result::getBestFound,
             py::return_value_policy::reference)
        .def("get_statistics",
             &Result::getStatistics,
             py::return_value_policy::reference)
        .def("get_iterations",
             &Result::getIterations,
             py::return_value_policy::reference)
        .def("get_run_time",
             &Result::getRunTime,
             py::return_value_policy::reference);
}
