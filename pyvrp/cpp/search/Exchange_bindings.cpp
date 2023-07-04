#include "Exchange.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(_Exchange, m)
{
    py::class_<LocalSearchOperator<Node>>(
        m, "NodeOperator", py::module_local());

    py::class_<Exchange<1, 0>, LocalSearchOperator<Node>>(m, "Exchange10")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
    py::class_<Exchange<2, 0>, LocalSearchOperator<Node>>(m, "Exchange20")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
    py::class_<Exchange<3, 0>, LocalSearchOperator<Node>>(m, "Exchange30")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
    py::class_<Exchange<1, 1>, LocalSearchOperator<Node>>(m, "Exchange11")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
    py::class_<Exchange<2, 1>, LocalSearchOperator<Node>>(m, "Exchange21")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<Exchange<3, 1>, LocalSearchOperator<Node>>(m, "Exchange31")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<Exchange<2, 2>, LocalSearchOperator<Node>>(m, "Exchange22")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<Exchange<3, 2>, LocalSearchOperator<Node>>(m, "Exchange32")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );

    py::class_<Exchange<3, 3>, LocalSearchOperator<Node>>(m, "Exchange33")
        .def(py::init<ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>()  // keep data alive
        );
}
