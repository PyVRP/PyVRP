#include "Matrix.h"
#include "bindings.h"

namespace py = pybind11;

void bind_Matrix(py::module_ &m)
{
    py::class_<Matrix<int>>(m, "IntMatrix")
        .def(py::init<size_t>(), py::arg("dimension"))
        .def(py::init<size_t, size_t>(), py::arg("n_rows"), py::arg("n_cols"))
        .def(py::init<std::vector<std::vector<int>>>(), py::arg("data"))
        .def("__getitem__",
             [](Matrix<int> &m, std::pair<size_t, size_t> idx) -> int {
                 return m(idx.first, idx.second);
             })
        .def("__setitem__",
             [](Matrix<int> &m, std::pair<size_t, size_t> idx, int value) {
                 m(idx.first, idx.second) = value;
             })
        .def("max", &Matrix<int>::max)
        .def("size", &Matrix<int>::size);

    py::class_<Matrix<double>>(m, "DoubleMatrix")
        .def(py::init<size_t>(), py::arg("dimension"))
        .def(py::init<size_t, size_t>(), py::arg("n_rows"), py::arg("n_cols"))
        .def(py::init<std::vector<std::vector<double>>>(), py::arg("data"))
        .def("__getitem__",
             [](Matrix<double> &m, std::pair<size_t, size_t> idx) -> double {
                 return m(idx.first, idx.second);
             })
        .def("__setitem__",
             [](Matrix<double> &m,
                std::pair<size_t, size_t> idx,
                double value) { m(idx.first, idx.second) = value; })
        .def("max", &Matrix<double>::max)
        .def("size", &Matrix<double>::size);
}
