#include "DynamicBitset.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_DynamicBitset, m)
{
    py::class_<DynamicBitset>(m, "DynamicBitset")
        .def(py::init<size_t>(), py::arg("num_bits"))
        .def("count", &DynamicBitset::count)
        .def("__len__", &DynamicBitset::size)
        .def(
            "__getitem__",
            [](DynamicBitset const &bitset, size_t idx) { return bitset[idx]; },
            py::arg("idx"))
        .def(
            "__setitem__",
            [](DynamicBitset &bitset, size_t idx, bool value) {
                bitset[idx] = value;
            },
            py::arg("idx"),
            py::arg("value"));
}
