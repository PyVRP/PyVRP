#include "Statistics.h"
#include "bindings.h"

namespace py = pybind11;

void bind_Statistics(py::module_ &m)
{
    py::class_<Statistics>(m, "Statistics")
        .def("num_iters", &Statistics::numIters)
        .def("run_times", &Statistics::runTimes)
        .def("iter_times", &Statistics::iterTimes)
        .def("feas_pop_size", &Statistics::feasPopSize)
        .def("feas_best_cost", &Statistics::feasBestCost)
        .def("feas_avg_cost", &Statistics::feasAvgCost)
        .def("feas_avg_num_routes", &Statistics::feasAvgNumRoutes)
        .def("infeas_pop_size", &Statistics::infeasPopSize)
        .def("infeas_best_cost", &Statistics::infeasBestCost)
        .def("infeas_avg_cost", &Statistics::infeasAvgCost)
        .def("infeas_avg_num_routes", &Statistics::infeasAvgNumRoutes)
        .def("incumbents", &Statistics::incumbents)
        .def("to_csv",
             &Statistics::toCsv,
             py::arg("path"),
             py::arg("sep") = ',');
}
