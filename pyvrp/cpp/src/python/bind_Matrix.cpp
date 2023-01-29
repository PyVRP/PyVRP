#include "Matrix.h"
#include "bindings.h"

namespace py = pybind11;

void bind_Matrix(py::module_ &m)
{
    py::class_<Matrix<int>>(m, "IntMatrix")
        .def(py::init<size_t>(), py::arg("dimension"))
        .def(py::init<size_t, size_t>(), py::arg("n_rows"), py::arg("n_cols"))
        .def(py::init<std::vector<std::vector<int>>>(), py::arg("data"));

    py::class_<Matrix<double>>(m, "DoubleMatrix")
        .def(py::init<size_t>(), py::arg("dimension"))
        .def(py::init<size_t, size_t>(), py::arg("n_rows"), py::arg("n_cols"))
        .def(py::init<std::vector<std::vector<double>>>(), py::arg("data"));
}
