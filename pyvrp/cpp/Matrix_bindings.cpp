#include "Matrix.h"
#include "precision.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(_Matrix, m)
{
    py::class_<Matrix<matrix_type>>(m, "Matrix")
        .def(py::init<size_t>(), py::arg("dimension"))
        .def(py::init<size_t, size_t>(), py::arg("n_rows"), py::arg("n_cols"))
        .def(py::init<std::vector<std::vector<matrix_type>>>(), py::arg("data"))
        .def(
            "__getitem__",
            [](Matrix<matrix_type> &m, std::pair<size_t, size_t> idx)
                -> matrix_type { return m(idx.first, idx.second); },
            py::arg("idx"))
        .def(
            "__setitem__",
            [](Matrix<matrix_type> &m,
               std::pair<size_t, size_t> idx,
               matrix_type value) { m(idx.first, idx.second) = value; },
            py::arg("idx"),
            py::arg("value"))
        .def("max", &Matrix<matrix_type>::max)
        .def("size", &Matrix<matrix_type>::size);
}
