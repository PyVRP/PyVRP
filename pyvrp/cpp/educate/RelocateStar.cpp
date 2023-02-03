#include "RelocateStar.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

void RelocateStar::init(Individual const &indiv)
{
    LocalSearchOperator<Route>::init(indiv);
    relocate.init(indiv);
}

int RelocateStar::evaluate(Route *U, Route *V)
{
    move = {};

    for (auto *nodeU = n(U->depot); !nodeU->isDepot(); nodeU = n(nodeU))
    {
        int deltaCost = relocate.evaluate(nodeU, V->depot);  // test after depot

        if (deltaCost < move.deltaCost)
            move = {deltaCost, nodeU, V->depot};

        for (auto *nodeV = n(V->depot); !nodeV->isDepot(); nodeV = n(nodeV))
        {
            deltaCost = relocate.evaluate(nodeU, nodeV);  // test U after V

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeU, nodeV};

            deltaCost = relocate.evaluate(nodeV, nodeU);  // test V after U

            if (deltaCost < move.deltaCost)
                move = {deltaCost, nodeV, nodeU};
        }
    }

    return move.deltaCost;
}

void RelocateStar::apply(Route *U, Route *V)
{
    move.from->insertAfter(move.to);
}

PYBIND11_MODULE(RelocateStar, m)
{
    py::class_<LocalSearchOperator<Route>>(
        m, "RouteOperator", py::module_local());

    py::class_<RelocateStar, LocalSearchOperator<Route>>(m, "RelocateStar")
        .def(py::init<ProblemData const &, PenaltyManager const &>(),
             py::arg("data"),
             py::arg("penalty_manager"));
}
