#include "CostEvaluator.h"
#include "DynamicBitset.h"
#include "Matrix.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Solution.h"
#include "SubPopulation.h"
#include "TimeWindowSegment.h"
#include "pyvrp_docs.h"

#include <pybind11/functional.h>
#include <pybind11/numpy.h>
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
using pyvrp::RandomNumberGenerator;
using pyvrp::Solution;
using pyvrp::SubPopulation;
using TWS = pyvrp::TimeWindowSegment;

namespace pybind11
{
namespace detail
{
template <typename T> struct type_caster<Matrix<T>>
{
public:
#ifdef PYVRP_DOUBLE_PRECISION
    PYBIND11_TYPE_CASTER(Matrix<T>, _("Matrix<double>"));
#else
    PYBIND11_TYPE_CASTER(Matrix<T>, _("Matrix<int>"));
#endif

    // Python -> C++
    bool load(py::handle src, bool)
    {
        static_assert(sizeof(T) == sizeof(pyvrp::Value));

        auto const style = py::array::c_style | py::array::forcecast;
        auto const buf = py::array_t<pyvrp::Value, style>::ensure(src);

        if (!buf || buf.ndim() != 2)
            throw py::value_error("Expected 2D np.ndarray argument!");

        std::vector<T> data = {buf.data(), buf.data() + buf.size()};
        value = Matrix<T>(data, buf.shape(0), buf.shape(1));

        return true;
    }

    // C++ -> Python
    static py::handle cast(Matrix<T> const &src,
                           [[maybe_unused]] py::return_value_policy policy,
                           [[maybe_unused]] py::handle parent)
    {
        static_assert(sizeof(T) == sizeof(pyvrp::Value));
        auto constexpr elemSize = sizeof(pyvrp::Value);

        py::array a(
            py::dtype::of<pyvrp::Value>(),                        // dtype
            {src.numRows(), src.numCols()},                       // shape
            {elemSize * src.numCols(), elemSize},                 // strides
            reinterpret_cast<pyvrp::Value const *>(src.data()));  // data

        return a.release();
    }
};
}  // namespace detail
}  // namespace pybind11

PYBIND11_MODULE(_pyvrp, m)
{
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

    py::class_<ProblemData::Client>(
        m, "Client", DOC(pyvrp, ProblemData, Client))
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

    py::class_<ProblemData::VehicleType>(
        m, "VehicleType", DOC(pyvrp, ProblemData, VehicleType))
        .def(py::init<pyvrp::Value, size_t>(),
             py::arg("capacity"),
             py::arg("num_available"))
        .def_property_readonly("capacity",
                               [](ProblemData::VehicleType const &vehicleType) {
                                   return vehicleType.capacity.get();
                               })
        .def_readonly("num_available", &ProblemData::VehicleType::numAvailable)
        .def_readonly("depot", &ProblemData::VehicleType::depot);

    py::class_<ProblemData>(m, "ProblemData", DOC(pyvrp, ProblemData))
        .def(py::init<std::vector<ProblemData::Client> const &,
                      std::vector<ProblemData::VehicleType> const &,
                      Matrix<pyvrp::Distance>,
                      Matrix<pyvrp::Duration>>(),
             py::arg("clients"),
             py::arg("vehicle_types"),
             py::arg("distance_matrix"),
             py::arg("duration_matrix"))
        .def_property_readonly("num_clients",
                               &ProblemData::numClients,
                               DOC(pyvrp, ProblemData, numClients))
        .def_property_readonly("num_vehicle_types",
                               &ProblemData::numVehicleTypes,
                               DOC(pyvrp, ProblemData, numVehicleTypes))
        .def_property_readonly("num_vehicles",
                               &ProblemData::numVehicles,
                               DOC(pyvrp, ProblemData, numVehicles))
        .def("client",
             &ProblemData::client,
             py::arg("client"),
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, client))
        .def("centroid",
             &ProblemData::centroid,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, centroid))
        .def("vehicle_type",
             &ProblemData::vehicleType,
             py::arg("vehicle_type"),
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, vehicleType))
        .def("distance_matrix",
             &ProblemData::distanceMatrix,
             DOC(pyvrp, ProblemData, distanceMatrix))
        .def("duration_matrix",
             &ProblemData::durationMatrix,
             DOC(pyvrp, ProblemData, durationMatrix))
        .def(
            "dist",
            [](ProblemData const &data, size_t first, size_t second) {
                return data.dist(first, second).get();
            },
            py::arg("first"),
            py::arg("second"),
            DOC(pyvrp, ProblemData, dist))
        .def(
            "duration",
            [](ProblemData const &data, size_t first, size_t second) {
                return data.duration(first, second).get();
            },
            py::arg("first"),
            py::arg("second"),
            DOC(pyvrp, ProblemData, duration));

    py::class_<Solution::Route>(m, "Route", DOC(pyvrp, Solution, Route))
        .def(py::init<ProblemData const &, std::vector<size_t>, size_t>(),
             py::arg("data"),
             py::arg("visits"),
             py::arg("vehicle_type"))
        .def("visits",
             &Solution::Route::visits,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Solution, Route, visits))
        .def(
            "distance",
            [](Solution::Route const &route) { return route.distance().get(); },
            DOC(pyvrp, Solution, Route, distance))
        .def(
            "demand",
            [](Solution::Route const &route) { return route.demand().get(); },
            DOC(pyvrp, Solution, Route, demand))
        .def(
            "excess_load",
            [](Solution::Route const &route) {
                return route.excessLoad().get();
            },
            DOC(pyvrp, Solution, Route, excessLoad))
        .def(
            "duration",
            [](Solution::Route const &route) { return route.duration().get(); },
            DOC(pyvrp, Solution, Route, duration))
        .def(
            "time_warp",
            [](Solution::Route const &route) { return route.timeWarp().get(); },
            DOC(pyvrp, Solution, Route, timeWarp))
        .def(
            "start_time",
            [](Solution::Route const &route) {
                return route.startTime().get();
            },
            DOC(pyvrp, Solution, Route, startTime))
        .def(
            "end_time",
            [](Solution::Route const &route) { return route.endTime().get(); },
            DOC(pyvrp, Solution, Route, endTime))
        .def(
            "slack",
            [](Solution::Route const &route) { return route.slack().get(); },
            DOC(pyvrp, Solution, Route, slack))
        .def(
            "travel_duration",
            [](Solution::Route const &route) {
                return route.travelDuration().get();
            },
            DOC(pyvrp, Solution, Route, travelDuration))
        .def(
            "service_duration",
            [](Solution::Route const &route) {
                return route.serviceDuration().get();
            },
            DOC(pyvrp, Solution, Route, serviceDuration))
        .def(
            "wait_duration",
            [](Solution::Route const &route) {
                return route.waitDuration().get();
            },
            DOC(pyvrp, Solution, Route, waitDuration))
        .def(
            "release_time",
            [](Solution::Route const &route) {
                return route.releaseTime().get();
            },
            DOC(pyvrp, Solution, Route, releaseTime))
        .def(
            "prizes",
            [](Solution::Route const &route) { return route.prizes().get(); },
            DOC(pyvrp, Solution, Route, prizes))
        .def("centroid",
             &Solution::Route::centroid,
             DOC(pyvrp, Solution, Route, centroid))
        .def("vehicle_type",
             &Solution::Route::vehicleType,
             DOC(pyvrp, Solution, Route, vehicleType))
        .def("is_feasible", &Solution::Route::isFeasible)
        .def("has_excess_load", &Solution::Route::hasExcessLoad)
        .def("has_time_warp", &Solution::Route::hasTimeWarp)
        .def("__len__", &Solution::Route::size)
        .def(
            "__iter__",
            [](Solution::Route const &route) {
                return py::make_iterator(route.begin(), route.end());
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
        .def(py::pickle(
            [](Solution::Route const &route) {  // __getstate__
                // Returns a tuple that completely encodes the route's state.
                return py::make_tuple(route.visits(),
                                      route.distance().get(),
                                      route.demand().get(),
                                      route.excessLoad().get(),
                                      route.duration().get(),
                                      route.timeWarp().get(),
                                      route.travelDuration().get(),
                                      route.serviceDuration().get(),
                                      route.waitDuration().get(),
                                      route.releaseTime().get(),
                                      route.startTime().get(),
                                      route.slack().get(),
                                      route.prizes().get(),
                                      route.centroid(),
                                      route.vehicleType());
            },
            [](py::tuple t) {  // __setstate__
                Solution::Route route = Solution::Route(
                    t[0].cast<std::vector<size_t>>(),         // visits
                    t[1].cast<pyvrp::Value>(),                // distance
                    t[2].cast<pyvrp::Value>(),                // demand
                    t[3].cast<pyvrp::Value>(),                // excess load
                    t[4].cast<pyvrp::Value>(),                // duration
                    t[5].cast<pyvrp::Value>(),                // time warp
                    t[6].cast<pyvrp::Value>(),                // travel
                    t[7].cast<pyvrp::Value>(),                // service
                    t[8].cast<pyvrp::Value>(),                // wait
                    t[9].cast<pyvrp::Value>(),                // release
                    t[10].cast<pyvrp::Value>(),               // start time
                    t[11].cast<pyvrp::Value>(),               // slack
                    t[12].cast<pyvrp::Value>(),               // prizes
                    t[13].cast<std::pair<double, double>>(),  // centroid
                    t[14].cast<size_t>());                    // vehicle type

                return route;
            }))
        .def("__str__", [](Solution::Route const &route) {
            std::stringstream stream;
            stream << route;
            return stream.str();
        });

    py::class_<Solution>(m, "Solution", DOC(pyvrp, Solution))
        // Note, the order of constructors is important! Since Solution::Route
        // implements __len__ and __getitem__, it can also be converted to
        // std::vector<size_t> and thus a list of Routes is a valid argument
        // for both constructors. We want to avoid using the second constructor
        // since that would lose the vehicle types associations. As pybind11
        // will use the first matching constructor we put this one first.
        .def(py::init<ProblemData const &,
                      std::vector<Solution::Route> const &>(),
             py::arg("data"),
             py::arg("routes"))
        .def(py::init<ProblemData const &,
                      std::vector<std::vector<size_t>> const &>(),
             py::arg("data"),
             py::arg("routes"))
        .def_property_readonly_static(
            "make_random",            // this is a bit of a workaround for
            [](py::object)            // classmethods, because pybind does
            {                         // not yet support those natively.
                py::options options;  // See issue 1693 in the pybind repo.
                options.disable_function_signatures();

                return py::cpp_function(
                    [](ProblemData const &data, RandomNumberGenerator &rng) {
                        return Solution(data, rng);
                    },
                    py::arg("data"),
                    py::arg("rng"),
                    DOC(pyvrp, Solution, Solution, 1));
            })
        .def(
            "num_routes", &Solution::numRoutes, DOC(pyvrp, Solution, numRoutes))
        .def("num_clients",
             &Solution::numClients,
             DOC(pyvrp, Solution, numClients))
        .def("num_missing_clients",
             &Solution::numMissingClients,
             DOC(pyvrp, Solution, numMissingClients))
        .def("get_routes",
             &Solution::getRoutes,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Solution, getRoutes))
        .def("get_neighbours",
             &Solution::getNeighbours,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Solution, getNeighbours))
        .def("is_feasible",
             &Solution::isFeasible,
             DOC(pyvrp, Solution, isFeasible))
        .def("is_complete",
             &Solution::isComplete,
             DOC(pyvrp, Solution, isComplete))
        .def("has_excess_load",
             &Solution::hasExcessLoad,
             DOC(pyvrp, Solution, hasExcessLoad))
        .def("has_time_warp",
             &Solution::hasTimeWarp,
             DOC(pyvrp, Solution, hasTimeWarp))
        .def(
            "distance",
            [](Solution const &sol) { return sol.distance().get(); },
            DOC(pyvrp, Solution, distance))
        .def(
            "excess_load",
            [](Solution const &sol) { return sol.excessLoad().get(); },
            DOC(pyvrp, Solution, excessLoad))
        .def(
            "time_warp",
            [](Solution const &sol) { return sol.timeWarp().get(); },
            DOC(pyvrp, Solution, timeWarp))
        .def(
            "prizes",
            [](Solution const &sol) { return sol.prizes().get(); },
            DOC(pyvrp, Solution, prizes))
        .def(
            "uncollected_prizes",
            [](Solution const &sol) { return sol.uncollectedPrizes().get(); },
            DOC(pyvrp, Solution, uncollectedPrizes))
        .def("__copy__", [](Solution const &sol) { return Solution(sol); })
        .def(
            "__deepcopy__",
            [](Solution const &sol, py::dict) { return Solution(sol); },
            py::arg("memo"))
        .def("__hash__",
             [](Solution const &sol) { return std::hash<Solution>()(sol); })
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](Solution const &sol) {  // __getstate__
                // Returns a tuple that completely encodes the solution's state.
                return py::make_tuple(sol.numClients(),
                                      sol.numMissingClients(),
                                      sol.distance().get(),
                                      sol.excessLoad().get(),
                                      sol.prizes().get(),
                                      sol.uncollectedPrizes().get(),
                                      sol.timeWarp().get(),
                                      sol.getRoutes(),
                                      sol.getNeighbours());
            },
            [](py::tuple t) {  // __setstate__
                using Routes = std::vector<Solution::Route>;
                using Neighbours
                    = std::vector<std::optional<std::pair<size_t, size_t>>>;

                Solution sol
                    = Solution(t[0].cast<size_t>(),        // num clients
                               t[1].cast<size_t>(),        // num missing
                               t[2].cast<pyvrp::Value>(),  // distance
                               t[3].cast<pyvrp::Value>(),  // excess load
                               t[4].cast<pyvrp::Value>(),  // prizes
                               t[5].cast<pyvrp::Value>(),  // uncollected
                               t[6].cast<pyvrp::Value>(),  // time warp
                               t[7].cast<Routes>(),        // routes
                               t[8].cast<Neighbours>());   // neighbours

                return sol;
            }))
        .def("__str__", [](Solution const &sol) {
            std::stringstream stream;
            stream << sol;
            return stream.str();
        });

    py::class_<CostEvaluator>(m, "CostEvaluator", DOC(pyvrp, CostEvaluator))
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
            py::arg("capacity"),
            DOC(pyvrp, CostEvaluator, loadPenalty))
        .def(
            "tw_penalty",
            [](CostEvaluator const &evaluator, pyvrp::Value const timeWarp) {
                return evaluator.twPenalty(timeWarp).get();
            },
            py::arg("time_warp"),
            DOC(pyvrp, CostEvaluator, twPenalty))
        .def(
            "penalised_cost",
            [](CostEvaluator const &evaluator, Solution const &solution) {
                return evaluator.penalisedCost(solution).get();
            },
            py::arg("solution"),
            DOC(pyvrp, CostEvaluator, penalisedCost))
        .def(
            "cost",
            [](CostEvaluator const &evaluator, Solution const &solution) {
                return evaluator.cost(solution).get();
            },
            py::arg("solution"),
            DOC(pyvrp, CostEvaluator, cost));

    py::class_<PopulationParams>(
        m, "PopulationParams", DOC(pyvrp, PopulationParams))
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
                      py::return_value_policy::reference_internal,
                      R"doc(
                            Solution for this SubPopulationItem.

                            Returns
                            -------
                            Solution
                                Solution for this SubPopulationItem.
                      )doc")
        .def_readonly("fitness",
                      &SubPopulation::Item::fitness,
                      R"doc(
                Fitness value for this SubPopulationItem.

                Returns
                -------
                float
                    Fitness value for this SubPopulationItem.

                .. warning::

                This is a cached property that is not automatically updated.
                Before accessing the property, 
                :meth:`~SubPopulation.update_fitness` should be called unless 
                the population has not changed since the last call.
            )doc")
        .def("avg_distance_closest",
             &SubPopulation::Item::avgDistanceClosest,
             R"doc(
                Determines the average distance of the solution wrapped by this
                item to a number of solutions that are most similar to it. This 
                provides a measure of the relative 'diversity' of the wrapped
                solution.

                Returns
                -------
                float
                    The average distance/diversity of the wrapped solution.
             )doc");

    py::class_<SubPopulation>(m, "SubPopulation", DOC(pyvrp, SubPopulation))
        .def(py::init<pyvrp::diversity::DiversityMeasure,
                      PopulationParams const &>(),
             py::arg("diversity_op"),
             py::arg("params"),
             py::keep_alive<1, 3>())  // keep params alive
        .def("add",
             &SubPopulation::add,
             py::arg("solution"),
             py::arg("cost_evaluator"),
             DOC(pyvrp, SubPopulation, add))
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
        .def("purge",
             &SubPopulation::purge,
             py::arg("cost_evaluator"),
             DOC(pyvrp, SubPopulation, purge))
        .def("update_fitness",
             &SubPopulation::updateFitness,
             py::arg("cost_evaluator"),
             DOC(pyvrp, SubPopulation, updateFitness));

    py::class_<TWS>(m, "TimeWindowSegment", DOC(pyvrp, TimeWindowSegment))
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
        .def(
            "duration",
            [](TWS const &tws) { return tws.duration().get(); },
            DOC(pyvrp, TimeWindowSegment, duration))
        .def(
            "tw_early",
            [](TWS const &tws) { return tws.twEarly().get(); },
            DOC(pyvrp, TimeWindowSegment, twEarly))
        .def(
            "tw_late",
            [](TWS const &tws) { return tws.twLate().get(); },
            DOC(pyvrp, TimeWindowSegment, twLate))
        .def(
            "total_time_warp",
            [](TWS const &tws) { return tws.totalTimeWarp().get(); },
            DOC(pyvrp, TimeWindowSegment, totalTimeWarp))
        .def_static("merge",
                    &TWS::merge<>,
                    py::arg("duration_matrix"),
                    py::arg("first"),
                    py::arg("second"))
        .def_static("merge",
                    &TWS::merge<TWS>,
                    py::arg("duration_matrix"),
                    py::arg("first"),
                    py::arg("second"),
                    py::arg("third"));

    py::class_<RandomNumberGenerator>(
        m, "RandomNumberGenerator", DOC(pyvrp, RandomNumberGenerator))
        .def(py::init<uint32_t>(), py::arg("seed"))
        .def("min", &RandomNumberGenerator::min)
        .def("max", &RandomNumberGenerator::max)
        .def("__call__", &RandomNumberGenerator::operator())
        .def("rand", &RandomNumberGenerator::rand<double>)
        .def("randint", &RandomNumberGenerator::randint<int>, py::arg("high"));
}
