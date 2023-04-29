#include "TimeWindowSegment.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;
using TWS = TimeWindowSegment;

PYBIND11_MODULE(_TimeWindowSegment, m)
{
    py::class_<TWS>(m, "TimeWindowSegment")
        .def(py::init<int, int, int, int, int, int>(),
             py::arg("idx_first"),
             py::arg("idx_last"),
             py::arg("duration"),
             py::arg("time_warp"),
             py::arg("tw_early"),
             py::arg("tw_late"))
        .def("total_time_warp", &TWS::totalTimeWarp)
        .def_static("merge",
                    &TWS::merge<>,
                    py::arg("duration_matrix"),
                    py::arg("first"),
                    py::arg("second"))
        .def_static("merge",
                    &TWS::merge<TWS>,
                    py::arg("duration_matrix"),
                    py::arg("first"),
                    py::arg("second"),
                    py::arg("third"))
        .def_static("merge",
                    &TWS::merge<TWS, TWS>,
                    py::arg("duration_matrix"),
                    py::arg("first"),
                    py::arg("second"),
                    py::arg("third"),
                    py::arg("fourth"));
}
