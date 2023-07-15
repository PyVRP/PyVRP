#include "CostEvaluator.h"
#include "DynamicBitset.h"
#include "Matrix.h"
#include "ProblemData.h"
#include "Solution.h"
#include "SubPopulation.h"
#include "TimeWindowSegment.h"
#include "XorShift128.h"

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>

namespace py = pybind11;

using pyvrp::CostEvaluator;
using pyvrp::DynamicBitset;
using pyvrp::Matrix;
using pyvrp::PopulationParams;
using pyvrp::ProblemData;
using pyvrp::Solution;
using pyvrp::SubPopulation;
using pyvrp::XorShift128;
using TWS = pyvrp::TimeWindowSegment;

template <typename... Args>
TWS merge(Matrix<pyvrp::Value> const &mat, Args... args)
{
    Matrix<pyvrp::Duration> durMat(mat.numRows(), mat.numCols());

    // Copy the Matrix<pyvrp::Value> over to Matrix<Duration>. That's not
    // efficient, but since this class is internal to PyVRP that does not matter
    // much. We only expose it to Python for testing.
    for (size_t row = 0; row != durMat.numRows(); ++row)
        for (size_t col = 0; col != durMat.numCols(); ++col)
            durMat(row, col) = mat(row, col);

    return TWS::merge(durMat, args...);
}

PYBIND11_MODULE(_pyvrp, m)
{
    py::class_<CostEvaluator>(m, "CostEvaluator")
        .def(py::init([](unsigned int capacityPenalty, unsigned int twPenalty) {
                 return CostEvaluator(capacityPenalty, twPenalty);
             }),
             py::arg("capacity_penalty") = 0,
             py::arg("tw_penalty") = 0)
        .def(
            "load_penalty",
            [](CostEvaluator const &evaluator,
               pyvrp::Value load,
               pyvrp::Value capacity) {
                return evaluator.loadPenalty(load, capacity).get();
            },
            py::arg("load"),
            py::arg("capacity"))
        .def(
            "tw_penalty",
            [](CostEvaluator const &evaluator, pyvrp::Value const timeWarp) {
                return evaluator.twPenalty(timeWarp).get();
            },
            py::arg("time_warp"))
        .def(
            "penalised_cost",
            [](CostEvaluator const &evaluator, Solution const &solution) {
                return evaluator.penalisedCost(solution).get();
            },
            py::arg("solution"))
        .def(
            "cost",
            [](CostEvaluator const &evaluator, Solution const &solution) {
                return evaluator.cost(solution).get();
            },
            py::arg("solution"));

    py::class_<DynamicBitset>(m, "DynamicBitset")
        .def(py::init<size_t>(), py::arg("num_bits"))
        .def(py::self == py::self, py::arg("other"))  // this is __eq__
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
            py::arg("value"))
        .def("__or__", &DynamicBitset::operator|, py::arg("other"))
        .def("__and__", &DynamicBitset::operator&, py::arg("other"))
        .def("__xor__", &DynamicBitset::operator^, py::arg("other"))
        .def("__invert__", &DynamicBitset::operator~);

    py::class_<Matrix<pyvrp::Value>>(m, "Matrix")
        .def(py::init<size_t>(), py::arg("dimension"))
        .def(py::init<size_t, size_t>(), py::arg("n_rows"), py::arg("n_cols"))
        .def(py::init<std::vector<std::vector<pyvrp::Value>>>(),
             py::arg("data"))
        .def_property_readonly("num_cols", &Matrix<pyvrp::Value>::numCols)
        .def_property_readonly("num_rows", &Matrix<pyvrp::Value>::numRows)
        .def(
            "__getitem__",
            [](Matrix<pyvrp::Value> &m, std::pair<size_t, size_t> idx)
                -> pyvrp::Value { return m(idx.first, idx.second); },
            py::arg("idx"))
        .def(
            "__setitem__",
            [](Matrix<pyvrp::Value> &m,
               std::pair<size_t, size_t> idx,
               pyvrp::Value value) { m(idx.first, idx.second) = value; },
            py::arg("idx"),
            py::arg("value"))
        .def("max", &Matrix<pyvrp::Value>::max)
        .def("size", &Matrix<pyvrp::Value>::size);

    py::class_<ProblemData::Client>(m, "Client")
        .def(py::init<pyvrp::Value,
                      pyvrp::Value,
                      pyvrp::Value,
                      pyvrp::Value,
                      pyvrp::Value,
                      pyvrp::Value,
                      pyvrp::Value,
                      pyvrp::Value,
                      bool>(),
             py::arg("x"),
             py::arg("y"),
             py::arg("demand") = 0,
             py::arg("service_duration") = 0,
             py::arg("tw_early") = 0,
             py::arg("tw_late") = 0,
             py::arg("release_time") = 0,
             py::arg("prize") = 0,
             py::arg("required") = true)
        .def_property_readonly(
            "x",
            [](ProblemData::Client const &client) { return client.x.get(); })
        .def_property_readonly(
            "y",
            [](ProblemData::Client const &client) { return client.y.get(); })
        .def_property_readonly("demand",
                               [](ProblemData::Client const &client) {
                                   return client.demand.get();
                               })
        .def_property_readonly("service_duration",
                               [](ProblemData::Client const &client) {
                                   return client.serviceDuration.get();
                               })
        .def_property_readonly("tw_early",
                               [](ProblemData::Client const &client) {
                                   return client.twEarly.get();
                               })
        .def_property_readonly("tw_late",
                               [](ProblemData::Client const &client) {
                                   return client.twLate.get();
                               })
        .def_property_readonly("release_time",
                               [](ProblemData::Client const &client) {
                                   return client.releaseTime.get();
                               })
        .def_property_readonly("prize",
                               [](ProblemData::Client const &client) {
                                   return client.prize.get();
                               })
        .def_readonly("required", &ProblemData::Client::required);

    py::class_<ProblemData::VehicleType>(m, "VehicleType")
        .def(py::init<pyvrp::Value, size_t>(),
             py::arg("capacity"),
             py::arg("num_available"))
        .def_property_readonly("capacity",
                               [](ProblemData::VehicleType const &vehicleType) {
                                   return vehicleType.capacity.get();
                               })
        .def_readonly("num_available", &ProblemData::VehicleType::numAvailable)
        .def_readonly("depot", &ProblemData::VehicleType::depot);

    py::class_<ProblemData>(m, "ProblemData")
        .def(py::init(
                 [](std::vector<ProblemData::Client> const &clients,
                    std::vector<ProblemData::VehicleType> const &vehicleTypes,
                    std::vector<std::vector<pyvrp::Value>> const &dist,
                    std::vector<std::vector<pyvrp::Value>> const &dur) {
                     Matrix<pyvrp::Distance> distMat(clients.size());
                     Matrix<pyvrp::Duration> durMat(clients.size());

                     for (size_t row = 0; row != clients.size(); ++row)
                         for (size_t col = 0; col != clients.size(); ++col)
                         {
                             distMat(row, col) = dist[row][col];
                             durMat(row, col) = dur[row][col];
                         }

                     return ProblemData(clients, vehicleTypes, distMat, durMat);
                 }),
             py::arg("clients"),
             py::arg("vehicle_types"),
             py::arg("distance_matrix"),
             py::arg("duration_matrix"))
        .def_property_readonly("num_clients", &ProblemData::numClients)
        .def_property_readonly("num_vehicles", &ProblemData::numVehicles)
        .def_property_readonly("num_vehicle_types",
                               &ProblemData::numVehicleTypes)
        .def("client",
             &ProblemData::client,
             py::arg("client"),
             py::return_value_policy::reference_internal)
        .def("depot",
             &ProblemData::depot,
             py::return_value_policy::reference_internal)
        .def("centroid",
             &ProblemData::centroid,
             py::return_value_policy::reference_internal)
        .def("vehicle_type",
             &ProblemData::vehicleType,
             py::arg("vehicle_type"),
             py::return_value_policy::reference_internal)
        .def(
            "dist",
            [](ProblemData const &data, size_t first, size_t second) {
                return data.dist(first, second).get();
            },
            py::arg("first"),
            py::arg("second"))
        .def(
            "duration",
            [](ProblemData const &data, size_t first, size_t second) {
                return data.duration(first, second).get();
            },
            py::arg("first"),
            py::arg("second"));
    py::class_<Solution::Route>(m, "Route")
        .def(py::init<ProblemData const &, std::vector<int>, size_t>(),
             py::arg("data"),
             py::arg("visits"),
             py::arg("vehicle_type"))
        .def("visits",
             &Solution::Route::visits,
             py::return_value_policy::reference_internal)
        .def(
            "distance",
            [](Solution::Route const &route) { return route.distance().get(); })
        .def("demand",
             [](Solution::Route const &route) { return route.demand().get(); })
        .def("excess_load",
             [](Solution::Route const &route) {
                 return route.excessLoad().get();
             })
        .def(
            "duration",
            [](Solution::Route const &route) { return route.duration().get(); })
        .def("service_duration",
             [](Solution::Route const &route) {
                 return route.serviceDuration().get();
             })
        .def(
            "time_warp",
            [](Solution::Route const &route) { return route.timeWarp().get(); })
        .def("wait_duration",
             [](Solution::Route const &route) {
                 return route.waitDuration().get();
             })
        .def("release_time",
             [](Solution::Route const &route) {
                 return route.releaseTime().get();
             })
        .def("prizes",
             [](Solution::Route const &route) { return route.prizes().get(); })
        .def("centroid", &Solution::Route::centroid)
        .def("vehicle_type", &Solution::Route::vehicleType)
        .def("is_feasible", &Solution::Route::isFeasible)
        .def("has_excess_load", &Solution::Route::hasExcessLoad)
        .def("has_time_warp", &Solution::Route::hasTimeWarp)
        .def("__len__", &Solution::Route::size)
        .def(
            "__iter__",
            [](Solution::Route const &route) {
                return py::make_iterator(route.cbegin(), route.cend());
            },
            py::return_value_policy::reference_internal)
        .def(
            "__getitem__",
            [](Solution::Route const &route, int idx) {
                // int so we also support negative offsets from the end.
                idx = idx < 0 ? route.size() + idx : idx;
                if (idx < 0 || static_cast<size_t>(idx) >= route.size())
                    throw py::index_error();
                return route[idx];
            },
            py::arg("idx"))
        .def(py::self == py::self)  // this is __eq__
        .def("__str__", [](Solution::Route const &route) {
            std::stringstream stream;
            stream << route;
            return stream.str();
        });

    py::class_<Solution>(m, "Solution")
        // Note, the order of constructors is important! Since Solution::Route
        // implements __len__ and __getitem__, it can also be converted to
        // std::vector<int> and thus a list of Routes is a valid argument for
        // both constructors. We want to avoid using the second constructor
        // since that would lose the vehicle types associations. As pybind11
        // will use the first matching constructor we put this one first.
        .def(py::init<ProblemData const &,
                      std::vector<Solution::Route> const &>(),
             py::arg("data"),
             py::arg("routes"))
        .def(py::init<ProblemData const &,
                      std::vector<std::vector<int>> const &>(),
             py::arg("data"),
             py::arg("routes"))
        .def_property_readonly_static(
            "make_random",                // this is a bit of a workaround for
            [](py::object)                // classmethods, because pybind does
            {                             // not yet support those natively.
                return py::cpp_function(  // See issue 1693 in the pybind repo.
                    [](ProblemData const &data, XorShift128 &rng) {
                        return Solution(data, rng);
                    },
                    py::arg("data"),
                    py::arg("rng"));
            })
        .def("num_routes", &Solution::numRoutes)
        .def("num_clients", &Solution::numClients)
        .def("get_routes",
             &Solution::getRoutes,
             py::return_value_policy::reference_internal)
        .def("get_neighbours",
             &Solution::getNeighbours,
             py::return_value_policy::reference_internal)
        .def("is_feasible", &Solution::isFeasible)
        .def("has_excess_load", &Solution::hasExcessLoad)
        .def("has_time_warp", &Solution::hasTimeWarp)
        .def("distance",
             [](Solution const &sol) { return sol.distance().get(); })
        .def("excess_load",
             [](Solution const &sol) { return sol.excessLoad().get(); })
        .def("time_warp",
             [](Solution const &sol) { return sol.timeWarp().get(); })
        .def("prizes", [](Solution const &sol) { return sol.prizes().get(); })
        .def("uncollected_prizes",
             [](Solution const &sol) { return sol.uncollectedPrizes().get(); })
        .def("__copy__", [](Solution const &sol) { return Solution(sol); })
        .def(
            "__deepcopy__",
            [](Solution const &sol, py::dict) { return Solution(sol); },
            py::arg("memo"))
        .def("__hash__",
             [](Solution const &sol) { return std::hash<Solution>()(sol); })
        .def(py::self == py::self)  // this is __eq__
        .def("__str__", [](Solution const &sol) {
            std::stringstream stream;
            stream << sol;
            return stream.str();
        });

    py::class_<PopulationParams>(m, "PopulationParams")
        .def(py::init<size_t, size_t, size_t, size_t, double, double>(),
             py::arg("min_pop_size") = 25,
             py::arg("generation_size") = 40,
             py::arg("nb_elite") = 4,
             py::arg("nb_close") = 5,
             py::arg("lb_diversity") = 0.1,
             py::arg("ub_diversity") = 0.5)
        .def_readwrite("min_pop_size", &PopulationParams::minPopSize)
        .def_readwrite("generation_size", &PopulationParams::generationSize)
        .def_property_readonly("max_pop_size", &PopulationParams::maxPopSize)
        .def_readwrite("nb_elite", &PopulationParams::nbElite)
        .def_readwrite("nb_close", &PopulationParams::nbClose)
        .def_readwrite("lb_diversity", &PopulationParams::lbDiversity)
        .def_readwrite("ub_diversity", &PopulationParams::ubDiversity);

    py::class_<SubPopulation::Item>(m, "SubPopulationItem")
        .def_readonly("solution",
                      &SubPopulation::Item::solution,
                      py::return_value_policy::reference_internal)
        .def_readonly("fitness", &SubPopulation::Item::fitness)
        .def("avg_distance_closest", &SubPopulation::Item::avgDistanceClosest);

    py::class_<SubPopulation>(m, "SubPopulation")
        .def(py::init<pyvrp::diversity::DiversityMeasure,
                      PopulationParams const &>(),
             py::arg("diversity_op"),
             py::arg("params"),
             py::keep_alive<1, 3>())  // keep params alive
        .def("add",
             &SubPopulation::add,
             py::arg("solution"),
             py::arg("cost_evaluator"))
        .def("__len__", &SubPopulation::size)
        .def(
            "__getitem__",
            [](SubPopulation const &subPop, int idx) {
                // int so we also support negative offsets from the end.
                idx = idx < 0 ? subPop.size() + idx : idx;
                if (idx < 0 || static_cast<size_t>(idx) >= subPop.size())
                    throw py::index_error();
                return subPop[idx];
            },
            py::arg("idx"),
            py::return_value_policy::reference_internal)
        .def(
            "__iter__",
            [](SubPopulation const &subPop) {
                return py::make_iterator(subPop.cbegin(), subPop.cend());
            },
            py::return_value_policy::reference_internal)
        .def("purge", &SubPopulation::purge, py::arg("cost_evaluator"))
        .def("update_fitness",
             &SubPopulation::updateFitness,
             py::arg("cost_evaluator"));

    py::class_<TWS>(m, "TimeWindowSegment")
        .def(py::init<int,
                      int,
                      pyvrp::Value,
                      pyvrp::Value,
                      pyvrp::Value,
                      pyvrp::Value,
                      pyvrp::Value>(),
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

    py::class_<XorShift128>(m, "XorShift128")
        .def(py::init<uint32_t>(), py::arg("seed"))
        .def("min", &XorShift128::min)
        .def("max", &XorShift128::max)
        .def("__call__", &XorShift128::operator())
        .def("rand", &XorShift128::rand<double>)
        .def("randint", &XorShift128::randint<int>, py::arg("high"));
}
