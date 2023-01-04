#include "Config.h"
#include "Exchange.h"
#include "GeneticAlgorithm.h"
#include "Individual.h"
#include "LocalSearch.h"
#include "LocalSearchOperator.h"
#include "MaxIterations.h"
#include "MaxRuntime.h"
#include "MoveTwoClientsReversed.h"
#include "NoImprovement.h"
#include "Population.h"
#include "ProblemData.h"
#include "RelocateStar.h"
#include "Result.h"
#include "Statistics.h"
#include "StoppingCriterion.h"
#include "SwapStar.h"
#include "TimedNoImprovement.h"
#include "TwoOpt.h"
#include "XorShift128.h"
#include "crossover.h"

#include <pybind11/chrono.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(hgspy, m)
{
    py::class_<XorShift128>(m, "XorShift128")
        .def(py::init<int>(), py::arg("seed"));

    py::class_<Individual>(m, "Individual")
        .def(py::init<ProblemData &, XorShift128 &>(),
             py::arg("data"),
             py::arg("rng"))
        .def(py::init<ProblemData &, std::vector<std::vector<int>>>(),
             py::arg("data"),
             py::arg("routes"))
        .def("cost", &Individual::cost)
        .def("get_routes", &Individual::getRoutes)
        .def("get_neighbours", &Individual::getNeighbours)
        .def("is_feasible", &Individual::isFeasible)
        .def("has_excess_capacity", &Individual::hasExcessCapacity)
        .def("has_time_warp", &Individual::hasTimeWarp)
        .def("broken_pairs_distance", &Individual::brokenPairsDistance)
        .def("to_file", &Individual::toFile);

    py::class_<LocalSearch>(m, "LocalSearch")
        .def(py::init<ProblemData &, XorShift128 &>(),
             py::arg("data"),
             py::arg("rng"))
        .def("add_node_operator",
             static_cast<void (LocalSearch::*)(LocalSearchOperator<Node> &)>(
                 &LocalSearch::addNodeOperator),
             py::arg("op"))
        .def("add_route_operator",
             static_cast<void (LocalSearch::*)(LocalSearchOperator<Route> &)>(
                 &LocalSearch::addRouteOperator),
             py::arg("op"))
        .def("search", &LocalSearch::search, py::arg("indiv"))
        .def("intensify", &LocalSearch::intensify, py::arg("indiv"));

    py::class_<Config>(m, "Config")
        .def(py::init<int,
                      int,
                      bool,
                      size_t,
                      size_t,
                      size_t,
                      double,
                      double,
                      size_t,
                      size_t,
                      size_t,
                      double,
                      double,
                      size_t,
                      double,
                      size_t,
                      size_t,
                      size_t,
                      size_t,
                      int,
                      int,
                      bool,
                      size_t>(),
             py::arg("seed") = 0,
             py::arg("timeLimit") = INT_MAX,
             py::arg("collectStatistics") = false,
             py::arg("initialCapacityPenalty") = 20,
             py::arg("initialTimeWarpPenalty") = 6,
             py::arg("nbPenaltyManagement") = 47,
             py::arg("penaltyIncrease") = 1.34,
             py::arg("penaltyDecrease") = 0.32,
             py::arg("minPopSize") = 25,
             py::arg("generationSize") = 40,
             py::arg("nbElite") = 4,
             py::arg("lbDiversity") = 0.1,
             py::arg("ubDiversity") = 0.5,
             py::arg("nbClose") = 5,
             py::arg("targetFeasible") = 0.43,
             py::arg("repairProbability") = 79,
             py::arg("repairBooster") = 12,
             py::arg("selectProbability") = 90,
             py::arg("nbGranular") = 34,
             py::arg("weightWaitTime") = 18,
             py::arg("weightTimeWarp") = 20,
             py::arg("shouldIntensify") = true,
             py::arg("postProcessPathLength") = 7)
        .def_readonly("seed", &Config::seed)
        .def_readonly("timeLimit", &Config::timeLimit)
        .def_readonly("collectStatistics", &Config::collectStatistics)
        .def_readonly("initialCapacityPenalty", &Config::initialCapacityPenalty)
        .def_readonly("initialTimeWarpPenalty", &Config::initialTimeWarpPenalty)
        .def_readonly("nbPenaltyManagement", &Config::nbPenaltyManagement)
        .def_readonly("penaltyIncrease", &Config::penaltyIncrease)
        .def_readonly("penaltyDecrease", &Config::penaltyDecrease)
        .def_readonly("minPopSize", &Config::minPopSize)
        .def_readonly("generationSize", &Config::generationSize)
        .def_readonly("nbElite", &Config::nbElite)
        .def_readonly("lbDiversity", &Config::lbDiversity)
        .def_readonly("ubDiversity", &Config::ubDiversity)
        .def_readonly("nbClose", &Config::nbClose)
        .def_readonly("targetFeasible", &Config::targetFeasible)
        .def_readonly("repairProbability", &Config::repairProbability)
        .def_readonly("repairBooster", &Config::repairBooster)
        .def_readonly("selectProbability", &Config::selectProbability)
        .def_readonly("nbGranular", &Config::nbGranular)
        .def_readonly("weightWaitTime", &Config::weightWaitTime)
        .def_readonly("weightTimeWarp", &Config::weightTimeWarp)
        .def_readonly("postProcessPathLength", &Config::postProcessPathLength);

    py::class_<ProblemData>(m, "ProblemData")
        .def(py::init<Config const &,
                      std::vector<std::pair<int, int>> const &,
                      std::vector<int> const &,
                      int,
                      int,
                      std::vector<std::pair<int, int>> const &,
                      std::vector<int> const &,
                      std::vector<std::vector<int>> const &,
                      std::vector<int> const &>(),
             py::arg("config"),
             py::arg("coords"),
             py::arg("demands"),
             py::arg("nb_vehicles"),
             py::arg("vehicle_cap"),
             py::arg("time_windows"),
             py::arg("service_durations"),
             py::arg("duration_matrix"),
             py::arg("release_times"))
        .def("from_file", &ProblemData::fromFile);

    py::class_<Population>(m, "Population")
        .def(py::init<ProblemData &, XorShift128 &>(),
             py::arg("data"),
             py::arg("rng"))
        .def("add", &Population::add, py::arg("individual"));

    py::class_<Statistics>(m, "Statistics")
        .def("num_iters", &Statistics::numIters)
        .def("run_times", &Statistics::runTimes)
        .def("iter_times", &Statistics::iterTimes)
        .def("feas_pop_size", &Statistics::feasPopSize)
        .def("feas_avg_diversity", &Statistics::feasAvgDiversity)
        .def("feas_best_cost", &Statistics::feasBestCost)
        .def("feas_avg_cost", &Statistics::feasAvgCost)
        .def("feas_avg_num_routes", &Statistics::feasAvgNumRoutes)
        .def("infeas_pop_size", &Statistics::infeasPopSize)
        .def("infeas_avg_diversity", &Statistics::infeasAvgDiversity)
        .def("infeas_best_cost", &Statistics::infeasBestCost)
        .def("infeas_avg_cost", &Statistics::infeasAvgCost)
        .def("infeas_avg_num_routes", &Statistics::infeasAvgNumRoutes)
        .def("incumbents", &Statistics::incumbents)
        .def("to_csv",
             &Statistics::toCsv,
             py::arg("path"),
             py::arg("sep") = ',');

    py::class_<Result>(m, "Result")
        .def("get_best_found",
             &Result::getBestFound,
             py::return_value_policy::reference)
        .def("get_statistics",
             &Result::getStatistics,
             py::return_value_policy::reference)
        .def("get_iterations",
             &Result::getIterations,
             py::return_value_policy::reference)
        .def("get_run_time",
             &Result::getRunTime,
             py::return_value_policy::reference);

    py::class_<GeneticAlgorithm>(m, "GeneticAlgorithm")
        .def(py::init<ProblemData &,
                      XorShift128 &,
                      Population &,
                      LocalSearch &>(),
             py::arg("data"),
             py::arg("rng"),
             py::arg("population"),
             py::arg("local_search"))
        .def("add_crossover_operator",
             &GeneticAlgorithm::addCrossoverOperator,
             py::arg("op"))
        .def("run", &GeneticAlgorithm::run, py::arg("stop"));

    // Stopping criteria (as a submodule)
    py::module stop = m.def_submodule("stop");

    py::class_<StoppingCriterion>(stop, "StoppingCriterion");

    py::class_<MaxIterations, StoppingCriterion>(stop, "MaxIterations")
        .def(py::init<size_t>(), py::arg("max_iterations"))
        .def("__call__", &MaxIterations::operator(), py::arg("best_cost"));

    py::class_<MaxRuntime, StoppingCriterion>(stop, "MaxRuntime")
        .def(py::init<double>(), py::arg("max_runtime"))
        .def("__call__", &MaxRuntime::operator(), py::arg("best_cost"));

    py::class_<NoImprovement, StoppingCriterion>(stop, "NoImprovement")
        .def(py::init<size_t>(), py::arg("max_iterations"))
        .def("__call__", &NoImprovement::operator(), py::arg("best_cost"));

    py::class_<TimedNoImprovement, StoppingCriterion>(stop,
                                                      "TimedNoImprovement")
        .def(py::init<size_t, double>(),
             py::arg("max_iterations"),
             py::arg("max_runtime"))
        .def("__call__", &TimedNoImprovement::operator(), py::arg("best_cost"));

    // Crossover operators (as a submodule)
    py::module xOps = m.def_submodule("crossover");

    xOps.def("selective_route_exchange", &selectiveRouteExchange);

    // Local search operators (as a submodule)
    py::module lsOps = m.def_submodule("operators");

    py::class_<LocalSearchOperator<Node>>(lsOps, "NodeLocalSearchOperator");
    py::class_<LocalSearchOperator<Route>>(lsOps, "RouteLocalSearchOperator");

    py::class_<Exchange<1, 0>, LocalSearchOperator<Node>>(lsOps, "Exchange10")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<Exchange<2, 0>, LocalSearchOperator<Node>>(lsOps, "Exchange20")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<Exchange<3, 0>, LocalSearchOperator<Node>>(lsOps, "Exchange30")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<Exchange<1, 1>, LocalSearchOperator<Node>>(lsOps, "Exchange11")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<Exchange<2, 1>, LocalSearchOperator<Node>>(lsOps, "Exchange21")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<Exchange<3, 1>, LocalSearchOperator<Node>>(lsOps, "Exchange31")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<Exchange<2, 2>, LocalSearchOperator<Node>>(lsOps, "Exchange22")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<Exchange<3, 2>, LocalSearchOperator<Node>>(lsOps, "Exchange32")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<Exchange<3, 3>, LocalSearchOperator<Node>>(lsOps, "Exchange33")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<MoveTwoClientsReversed, LocalSearchOperator<Node>>(
        lsOps, "MoveTwoClientsReversed")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<TwoOpt, LocalSearchOperator<Node>>(lsOps, "TwoOpt")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<RelocateStar, LocalSearchOperator<Route>>(lsOps, "RelocateStar")
        .def(py::init<ProblemData const &>(), py::arg("data"));

    py::class_<SwapStar, LocalSearchOperator<Route>>(lsOps, "SwapStar")
        .def(py::init<ProblemData const &>(), py::arg("data"));
}
