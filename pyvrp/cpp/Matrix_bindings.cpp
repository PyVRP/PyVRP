#include "Matrix.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_Matrix, m)
{
    py::class_<Matrix<int>>(m, "Matrix")
        .def(py::init<size_t>(), py::arg("dimension"))
        .def(py::init<size_t, size_t>(), py::arg("n_rows"), py::arg("n_cols"))
        .def(py::init<std::vector<std::vector<int>>>(), py::arg("data"))
        .def_property_readonly("num_cols", &Matrix<int>::numCols)
        .def_property_readonly("num_rows", &Matrix<int>::numRows)
        .def(
            "__getitem__",
            [](Matrix<int> &m, std::pair<size_t, size_t> idx) -> int {
                return m(idx.first, idx.second);
            },
            py::arg("idx"))
        .def(
            "__setitem__",
            [](Matrix<int> &m, std::pair<size_t, size_t> idx, int value) {
                m(idx.first, idx.second) = value;
            },
            py::arg("idx"),
            py::arg("value"))
        .def("max", &Matrix<int>::max)
        .def("size", &Matrix<int>::size);
}
