#include "TimeWindowSegment.h"
#include "bindings.h"

namespace py = pybind11;
using TWS = TimeWindowSegment;

void bind_TimeWindowSegment(py::module_ &m)
{
    py::class_<TWS>(m, "TimeWindowSegment")
        .def(py::init<Matrix<int> const *, int, int, int, int, int, int, int>(),
             py::arg("dist"),
             py::arg("idx_first"),
             py::arg("idx_last"),
             py::arg("duration"),
             py::arg("time_warp"),
             py::arg("tw_early"),
             py::arg("tw_late"),
             py::arg("release"))
        .def("segment_time_warp", &TWS::segmentTimeWarp)
        .def("total_time_warp", &TWS::totalTimeWarp)
        .def("merge", &TWS::merge<>)
        .def("merge", &TWS::merge<TWS>)
        .def("merge", &TWS::merge<TWS, TWS>);
}
