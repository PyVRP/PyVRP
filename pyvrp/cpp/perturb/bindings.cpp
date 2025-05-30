#include "DestroyOperator.h"
#include "DestroyRepair.h"
#include "GreedyRepair.h"
#include "NeighbourRemoval.h"
#include "RepairOperator.h"
#include "perturb_docs.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using pyvrp::perturb::DestroyOperator;
using pyvrp::perturb::DestroyRepair;
using pyvrp::perturb::GreedyRepair;
using pyvrp::perturb::NeighbourRemoval;
using pyvrp::perturb::RepairOperator;

#include <pybind11/functional.h>

PYBIND11_MODULE(_perturb, m)
{
    py::class_<DestroyOperator>(
        m, "DestroyOperator", DOC(pyvrp, perturb, DestroyOperator));

    py::class_<RepairOperator>(
        m, "RepairOperator", DOC(pyvrp, perturb, RepairOperator));

    py::class_<NeighbourRemoval, DestroyOperator>(
        m, "NeighbourRemoval", DOC(pyvrp, perturb, NeighbourRemoval))
        .def(py::init<pyvrp::ProblemData const &, size_t const>(),
             py::arg("data"),
             py::arg("num_removals"),
             py::keep_alive<1, 2>())  // keep data alive
        .def("__call__",
             &NeighbourRemoval::operator(),
             py::arg("nodes"),
             py::arg("routes"),
             py::arg("cost_evaluator"),
             py::arg("neighbours"),
             py::arg("rng"),
             py::call_guard<py::gil_scoped_release>());

    py::class_<GreedyRepair, RepairOperator>(
        m, "GreedyRepair", DOC(pyvrp, perturb, GreedyRepair))
        .def(py::init<pyvrp::ProblemData const &, size_t>(),
             py::arg("data"),
             py::arg("skip_optional_probability") = 100,
             py::keep_alive<1, 2>())  // keep data alive
        .def("__call__",
             &GreedyRepair::operator(),
             py::arg("nodes"),
             py::arg("routes"),
             py::arg("cost_evaluator"),
             py::arg("neighbours"),
             py::arg("rng"),
             py::call_guard<py::gil_scoped_release>());

    py::class_<DestroyRepair>(
        m, "DestroyRepair", DOC(pyvrp, perturb, DestroyRepair))
        .def(py::init<pyvrp::ProblemData const &>(),
             py::arg("data"),
             py::keep_alive<1, 2>())  // keep data alive until DR is freed
        .def("add_destroy_operator",
             &DestroyRepair::addDestroyOperator,
             py::arg("op"),
             py::keep_alive<1, 2>())
        .def("add_repair_operator",
             &DestroyRepair::addRepairOperator,
             py::arg("op"),
             py::keep_alive<1, 2>())
        .def("__call__",
             &DestroyRepair::operator(),
             py::arg("solution"),
             py::arg("cost_evaluator"),
             py::arg("neighbours"),
             py::arg("rng"),
             py::call_guard<py::gil_scoped_release>());
}
