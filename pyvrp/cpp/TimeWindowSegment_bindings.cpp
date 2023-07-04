#include "TimeWindowSegment.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;
using TWS = TimeWindowSegment;

template <typename... Args> TWS merge(Matrix<int> const &mat, Args... args)
{
    Matrix<Duration> durMat(mat.numRows(), mat.numCols());

    // Copy the Matrix<int> over to Matrix<Duration>. That's not very efficient,
    // but since this class is internal to PyVRP that does not matter much. We
    // only expose it to Python for testing.
    for (size_t row = 0; row != durMat.numRows(); ++row)
        for (size_t col = 0; col != durMat.numCols(); ++col)
            durMat(row, col) = mat(row, col);

    return TWS::merge(durMat, args...);
}

PYBIND11_MODULE(_TimeWindowSegment, m)
{
    py::class_<TWS>(m, "TimeWindowSegment")
        .def(py::init<int, int, Value, Value, Value, Value, Value>(),
             py::arg("idx_first"),
             py::arg("idx_last"),
             py::arg("duration"),
             py::arg("time_warp"),
             py::arg("tw_early"),
             py::arg("tw_late"),
             py::arg("release_time"))
        .def("total_time_warp",
             [](TWS const &tws) { return tws.totalTimeWarp().get(); })
        .def_static("merge",
                    &merge<TWS, TWS>,
                    py::arg("duration_matrix"),
                    py::arg("first"),
                    py::arg("second"))
        .def_static("merge",
                    &merge<TWS, TWS, TWS>,
                    py::arg("duration_matrix"),
                    py::arg("first"),
                    py::arg("second"),
                    py::arg("third"));
}
