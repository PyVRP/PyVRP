#include "bindings.h"
#include "CostEvaluator.h"
#include "DurationCostFunction.h"
#include "DurationSegment.h"
#include "DynamicBitset.h"
#include "LoadSegment.h"
#include "Matrix.h"
#include "PiecewiseLinearFunction.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Route.h"
#include "Solution.h"
#include "Trip.h"
#include "pyvrp_docs.h"

#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cstdint>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

namespace py = pybind11;

using pyvrp::CostEvaluator;
using pyvrp::DurationCostFunction;
using pyvrp::DurationSegment;
using pyvrp::DynamicBitset;
using pyvrp::LoadSegment;
using pyvrp::Matrix;
using pyvrp::PiecewiseLinearFunction;
using pyvrp::ProblemData;
using pyvrp::RandomNumberGenerator;
using pyvrp::Route;
using pyvrp::Solution;
using pyvrp::Trip;

PYBIND11_MODULE(_pyvrp, m)
{
    py::class_<DynamicBitset>(m, "DynamicBitset", DOC(pyvrp, DynamicBitset))
        .def(py::init<size_t>(), py::arg("num_bits"))
        .def(py::self == py::self, py::arg("other"))  // this is __eq__
        .def("all", &DynamicBitset::all)
        .def("any", &DynamicBitset::any)
        .def("none", &DynamicBitset::none)
        .def("count", &DynamicBitset::count)
        .def("__len__", &DynamicBitset::size)
        .def("set", &DynamicBitset::set)
        .def("reset", &DynamicBitset::reset)
        .def(
            "__getitem__",
            [](DynamicBitset const &bitset, size_t idx)
            {
                if (idx >= bitset.size())
                    throw py::index_error();
                return bitset[idx];
            },
            py::arg("idx"))
        .def(
            "__setitem__",
            [](DynamicBitset &bitset, size_t idx, bool value)
            {
                if (idx >= bitset.size())
                    throw py::index_error();
                bitset[idx] = value;
            },
            py::arg("idx"),
            py::arg("value"))
        .def("__or__", &DynamicBitset::operator|, py::arg("other"))
        .def("__and__", &DynamicBitset::operator&, py::arg("other"))
        .def("__xor__", &DynamicBitset::operator^, py::arg("other"))
        .def("__invert__", &DynamicBitset::operator~)
        .def(py::pickle(
            [](DynamicBitset const &bitset) {  // __getstate__
                std::vector<unsigned long long> blocks;
                for (auto const &block : bitset.data())
                    blocks.push_back(block.to_ullong());
                return py::make_tuple(blocks);
            },
            [](py::tuple t) -> DynamicBitset  // __setstate__
            {
                auto const blocks
                    = t[0].cast<std::vector<unsigned long long>>();
                return std::vector<DynamicBitset::Block>(blocks.begin(),
                                                         blocks.end());
            }));

    py::class_<PiecewiseLinearFunction>(
        m, "PiecewiseLinearFunction", DOC(pyvrp, PiecewiseLinearFunction))
        .def(py::init<std::vector<int64_t>, std::vector<int64_t>, int64_t>(),
             py::arg("breakpoints") = std::vector<int64_t>{0},
             py::arg("slopes") = std::vector<int64_t>{0},
             py::arg("intercept") = 0)
        .def("__call__", &PiecewiseLinearFunction::operator(), py::arg("x"))
        .def_property_readonly("breakpoints",
                               &PiecewiseLinearFunction::breakpoints,
                               py::return_value_policy::reference_internal)
        .def_property_readonly("slopes",
                               &PiecewiseLinearFunction::slopes,
                               py::return_value_policy::reference_internal)
        .def_property_readonly("values",
                               &PiecewiseLinearFunction::values,
                               py::return_value_policy::reference_internal)
        .def_property_readonly("intercept", &PiecewiseLinearFunction::intercept)
        .def("is_zero",
             &PiecewiseLinearFunction::isZero,
             DOC(pyvrp, PiecewiseLinearFunction, isZero))
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](PiecewiseLinearFunction const &function)  // __getstate__
            {
                return py::make_tuple(function.breakpoints(),
                                      function.slopes(),
                                      function.intercept());
            },
            [](py::tuple t)  // __setstate__
            {
                return PiecewiseLinearFunction(
                    t[0].cast<std::vector<int64_t>>(),  // breakpoints
                    t[1].cast<std::vector<int64_t>>(),  // slopes
                    t[2].cast<int64_t>());              // intercept
            }));

    py::class_<DurationCostFunction>(
        m, "DurationCostFunction", DOC(pyvrp, DurationCostFunction))
        .def(py::init<std::vector<pyvrp::Duration>, std::vector<pyvrp::Cost>>(),
             py::arg("breakpoints") = std::vector<pyvrp::Duration>{0},
             py::arg("slopes") = std::vector<pyvrp::Cost>{0})
        .def(py::init<PiecewiseLinearFunction>(), py::arg("piecewise_linear"))
        // NOTE: #925/1044-FormPup41: Could be removed once agreed upon.
        // We intentionally do not expose DurationCostFunction::fromLinear() to
        // Python. It is an internal compatibility helper used to map legacy
        // scalar duration/overtime costs to a duration cost function. Exposing
        // it publicly would make deprecating or removing legacy scalar inputs
        // harder once users depend on the helper directly.
        .def("__call__", &DurationCostFunction::operator(), py::arg("duration"))
        .def_property_readonly("breakpoints",
                               &DurationCostFunction::breakpoints,
                               DOC(pyvrp, DurationCostFunction, breakpoints))
        .def_property_readonly("slopes",
                               &DurationCostFunction::slopes,
                               DOC(pyvrp, DurationCostFunction, slopes))
        .def_property_readonly("values",
                               &DurationCostFunction::values,
                               DOC(pyvrp, DurationCostFunction, values))
        .def_property_readonly(
            "piecewise_linear",
            &DurationCostFunction::piecewiseLinear,
            py::return_value_policy::reference_internal,
            DOC(pyvrp, DurationCostFunction, piecewiseLinear))
        .def_property_readonly("edge_cost_slope",
                               &DurationCostFunction::edgeCostSlope)
        .def("is_zero",
             &DurationCostFunction::isZero,
             DOC(pyvrp, DurationCostFunction, isZero))
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](DurationCostFunction const &function)  // __getstate__
            {
                return py::make_tuple(function.breakpoints(),
                                      function.slopes());
            },
            [](py::tuple t)  // __setstate__
            {
                if (t.size() == 2)
                {
                    return DurationCostFunction(
                        t[0].cast<
                            std::vector<pyvrp::Duration>>(),     // breakpoints
                        t[1].cast<std::vector<pyvrp::Cost>>());  // slopes
                }

                throw std::runtime_error("Invalid DurationCostFunction state.");
            }));

    py::class_<ProblemData::Client>(
        m, "Client", DOC(pyvrp, ProblemData, Client))
        .def(py::init<pyvrp::Coordinate,
                      pyvrp::Coordinate,
                      std::vector<pyvrp::Load>,
                      std::vector<pyvrp::Load>,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Cost,
                      bool,
                      std::optional<size_t>,
                      char const *>(),
             py::arg("x"),
             py::arg("y"),
             py::arg("delivery") = py::list(),
             py::arg("pickup") = py::list(),
             py::arg("service_duration") = 0,
             py::arg("tw_early") = 0,
             py::arg("tw_late") = std::numeric_limits<pyvrp::Duration>::max(),
             py::arg("release_time") = 0,
             py::arg("prize") = 0,
             py::arg("required") = true,
             py::arg("group") = py::none(),
             py::kw_only(),
             py::arg("name") = "")
        .def_readonly("x", &ProblemData::Client::x)
        .def_readonly("y", &ProblemData::Client::y)
        .def_readonly("delivery",
                      &ProblemData::Client::delivery,
                      py::return_value_policy::reference_internal)
        .def_readonly("pickup",
                      &ProblemData::Client::pickup,
                      py::return_value_policy::reference_internal)
        .def_readonly("service_duration", &ProblemData::Client::serviceDuration)
        .def_readonly("tw_early", &ProblemData::Client::twEarly)
        .def_readonly("tw_late", &ProblemData::Client::twLate)
        .def_readonly("release_time", &ProblemData::Client::releaseTime)
        .def_readonly("prize", &ProblemData::Client::prize)
        .def_readonly("required", &ProblemData::Client::required)
        .def_readonly("group", &ProblemData::Client::group)
        .def_readonly("name",
                      &ProblemData::Client::name,
                      py::return_value_policy::reference_internal)
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](ProblemData::Client const &client) {  // __getstate__
                return py::make_tuple(client.x,
                                      client.y,
                                      client.delivery,
                                      client.pickup,
                                      client.serviceDuration,
                                      client.twEarly,
                                      client.twLate,
                                      client.releaseTime,
                                      client.prize,
                                      client.required,
                                      client.group,
                                      client.name);
            },
            [](py::tuple t) {  // __setstate__
                ProblemData::Client client(
                    t[0].cast<pyvrp::Coordinate>(),         // x
                    t[1].cast<pyvrp::Coordinate>(),         // y
                    t[2].cast<std::vector<pyvrp::Load>>(),  // delivery
                    t[3].cast<std::vector<pyvrp::Load>>(),  // pickup
                    t[4].cast<pyvrp::Duration>(),           // service duration
                    t[5].cast<pyvrp::Duration>(),           // tw early
                    t[6].cast<pyvrp::Duration>(),           // tw late
                    t[7].cast<pyvrp::Duration>(),           // release time
                    t[8].cast<pyvrp::Cost>(),               // prize
                    t[9].cast<bool>(),                      // required
                    t[10].cast<std::optional<size_t>>(),    // group
                    t[11].cast<std::string>());             // name

                return client;
            }))
        .def(
            "__str__",
            [](ProblemData::Client const &client) { return client.name; },
            py::return_value_policy::reference_internal);

    py::class_<ProblemData::Depot>(m, "Depot", DOC(pyvrp, ProblemData, Depot))
        .def(py::init<pyvrp::Coordinate,
                      pyvrp::Coordinate,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      char const *>(),
             py::arg("x"),
             py::arg("y"),
             py::arg("tw_early") = 0,
             py::arg("tw_late") = std::numeric_limits<pyvrp::Duration>::max(),
             py::arg("service_duration") = 0,
             py::kw_only(),
             py::arg("name") = "")
        .def_readonly("x", &ProblemData::Depot::x)
        .def_readonly("y", &ProblemData::Depot::y)
        .def_readonly("tw_early", &ProblemData::Depot::twEarly)
        .def_readonly("tw_late", &ProblemData::Depot::twLate)
        .def_readonly("service_duration", &ProblemData::Depot::serviceDuration)
        .def_readonly("name",
                      &ProblemData::Depot::name,
                      py::return_value_policy::reference_internal)
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](ProblemData::Depot const &depot) {  // __getstate__
                return py::make_tuple(depot.x,
                                      depot.y,
                                      depot.twEarly,
                                      depot.twLate,
                                      depot.serviceDuration,
                                      depot.name);
            },
            [](py::tuple t) {  // __setstate__
                ProblemData::Depot depot(
                    t[0].cast<pyvrp::Coordinate>(),  // x
                    t[1].cast<pyvrp::Coordinate>(),  // y
                    t[2].cast<pyvrp::Duration>(),    // tw early
                    t[3].cast<pyvrp::Duration>(),    // tw late
                    t[4].cast<pyvrp::Duration>(),    // service duration
                    t[5].cast<std::string>());       // name

                return depot;
            }))
        .def(
            "__str__",
            [](ProblemData::Depot const &depot) { return depot.name; },
            py::return_value_policy::reference_internal);

    py::class_<ProblemData::ClientGroup>(
        m, "ClientGroup", DOC(pyvrp, ProblemData, ClientGroup))
        .def(py::init<std::vector<size_t>, bool, char const *>(),
             py::arg("clients") = py::list(),
             py::arg("required") = true,
             py::kw_only(),
             py::arg("name") = "")
        .def("add_client",
             &ProblemData::ClientGroup::addClient,
             py::arg("client"))
        .def("clear", &ProblemData::ClientGroup::clear)
        .def_property_readonly("clients",
                               &ProblemData::ClientGroup::clients,
                               py::return_value_policy::reference_internal)
        .def_readonly("required", &ProblemData::ClientGroup::required)
        .def_readonly("mutually_exclusive",
                      &ProblemData::ClientGroup::mutuallyExclusive)
        .def_readonly("name",
                      &ProblemData::ClientGroup::name,
                      py::return_value_policy::reference_internal)
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](ProblemData::ClientGroup const &group) {  // __getstate__
                return py::make_tuple(
                    group.clients(), group.required, group.name);
            },
            [](py::tuple t) {  // __setstate__
                ProblemData::ClientGroup group(
                    t[0].cast<std::vector<size_t>>(),  // clients
                    t[1].cast<bool>(),                 // required
                    t[2].cast<std::string>());         // name

                return group;
            }))
        .def("__len__", &ProblemData::ClientGroup::size)
        .def(
            "__iter__",
            [](ProblemData::ClientGroup const &group)
            { return py::make_iterator(group.begin(), group.end()); },
            py::return_value_policy::reference_internal)
        .def(
            "__str__",
            [](ProblemData::ClientGroup const &group) { return group.name; },
            py::return_value_policy::reference_internal);

    py::class_<ProblemData::VehicleType>(
        m, "VehicleType", DOC(pyvrp, ProblemData, VehicleType))
        .def(py::init<size_t,
                      std::vector<pyvrp::Load>,
                      size_t,
                      size_t,
                      pyvrp::Cost,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Distance,
                      pyvrp::Cost,
                      pyvrp::Cost,
                      size_t,
                      std::optional<pyvrp::Duration>,
                      std::vector<pyvrp::Load>,
                      std::vector<size_t>,
                      size_t,
                      pyvrp::Duration,
                      pyvrp::Cost,
                      std::optional<DurationCostFunction>,
                      char const *>(),
             py::arg("num_available") = 1,
             py::arg("capacity") = py::list(),
             py::arg("start_depot") = 0,
             py::arg("end_depot") = 0,
             py::arg("fixed_cost") = 0,
             py::arg("tw_early") = 0,
             py::arg("tw_late") = std::numeric_limits<pyvrp::Duration>::max(),
             py::arg("shift_duration")
             = std::numeric_limits<pyvrp::Duration>::max(),
             py::arg("max_distance")
             = std::numeric_limits<pyvrp::Distance>::max(),
             py::arg("unit_distance_cost") = 1,
             py::arg("unit_duration_cost") = 0,
             py::arg("profile") = 0,
             py::arg("start_late") = py::none(),
             py::arg("initial_load") = py::list(),
             py::arg("reload_depots") = py::list(),
             py::arg("max_reloads") = std::numeric_limits<size_t>::max(),
             py::arg("max_overtime") = 0,
             py::arg("unit_overtime_cost") = 0,
             py::arg("duration_cost_function") = py::none(),
             py::kw_only(),
             py::arg("name") = "")
        .def_readonly("num_available", &ProblemData::VehicleType::numAvailable)
        .def_readonly("capacity",
                      &ProblemData::VehicleType::capacity,
                      py::return_value_policy::reference_internal)
        .def_readonly("start_depot", &ProblemData::VehicleType::startDepot)
        .def_readonly("end_depot", &ProblemData::VehicleType::endDepot)
        .def_readonly("fixed_cost", &ProblemData::VehicleType::fixedCost)
        .def_readonly("tw_early", &ProblemData::VehicleType::twEarly)
        .def_readonly("tw_late", &ProblemData::VehicleType::twLate)
        .def_readonly("shift_duration",
                      &ProblemData::VehicleType::shiftDuration)
        .def_readonly("max_distance", &ProblemData::VehicleType::maxDistance)
        .def_readonly("unit_distance_cost",
                      &ProblemData::VehicleType::unitDistanceCost)
        .def_readonly("unit_duration_cost",
                      &ProblemData::VehicleType::unitDurationCost)
        .def_readonly("profile", &ProblemData::VehicleType::profile)
        .def_readonly("start_late", &ProblemData::VehicleType::startLate)
        .def_readonly("initial_load",
                      &ProblemData::VehicleType::initialLoad,
                      py::return_value_policy::reference_internal)
        .def_readonly("reload_depots",
                      &ProblemData::VehicleType::reloadDepots,
                      py::return_value_policy::reference_internal)
        .def_readonly("max_reloads", &ProblemData::VehicleType::maxReloads)
        .def_readonly("max_overtime", &ProblemData::VehicleType::maxOvertime)
        .def_readonly("unit_overtime_cost",
                      &ProblemData::VehicleType::unitOvertimeCost)
        .def_readonly("duration_cost_function",
                      &ProblemData::VehicleType::durationCostFunction)
        .def_property_readonly("duration_cost_slope",
                               &ProblemData::VehicleType::durationCostSlope)
        .def_readonly("max_duration", &ProblemData::VehicleType::maxDuration)
        .def_property_readonly("max_trips", &ProblemData::VehicleType::maxTrips)
        .def_readonly("name",
                      &ProblemData::VehicleType::name,
                      py::return_value_policy::reference_internal)
        // NOTE:
        // We bind replace() through a lambda instead of directly binding the
        // C++ method pointer. The Python API needs tri-state semantics for
        // duration_cost_function:
        //   1) argument omitted  -> keep current duration cost mode;
        //   2) explicitly None   -> clear custom function and switch to legacy;
        //   3) concrete function -> set/replace custom duration cost function.
        //
        // Pybind's regular optional binding cannot reliably distinguish (1)
        // from (2), so we parse a raw py::object and pass the explicit
        // durationCostFunctionProvided flag to VehicleType::replace().
        .def(
            "replace",
            [](ProblemData::VehicleType const &vehicleType,
               std::optional<size_t> numAvailable,
               std::optional<std::vector<pyvrp::Load>> capacity,
               std::optional<size_t> startDepot,
               std::optional<size_t> endDepot,
               std::optional<pyvrp::Cost> fixedCost,
               std::optional<pyvrp::Duration> twEarly,
               std::optional<pyvrp::Duration> twLate,
               std::optional<pyvrp::Duration> shiftDuration,
               std::optional<pyvrp::Distance> maxDistance,
               std::optional<pyvrp::Cost> unitDistanceCost,
               std::optional<pyvrp::Cost> unitDurationCost,
               std::optional<size_t> profile,
               std::optional<pyvrp::Duration> startLate,
               std::optional<std::vector<pyvrp::Load>> initialLoad,
               std::optional<std::vector<size_t>> reloadDepots,
               std::optional<size_t> maxReloads,
               std::optional<pyvrp::Duration> maxOvertime,
               std::optional<pyvrp::Cost> unitOvertimeCost,
               py::object durationCostFunction,
               std::optional<std::string> name)
            {
                // Ellipsis means: not provided from Python.
                auto const durationCostFunctionProvided
                    = !durationCostFunction.is(py::ellipsis());

                // Keep nullopt both when the argument is omitted and when it is
                // explicitly None; durationCostFunctionProvided disambiguates.
                std::optional<DurationCostFunction> parsedDurationCostFunction
                    = std::nullopt;
                if (durationCostFunctionProvided
                    && !durationCostFunction.is_none())
                {
                    parsedDurationCostFunction
                        = durationCostFunction.cast<DurationCostFunction>();
                }

                return vehicleType.replace(numAvailable,
                                           capacity,
                                           startDepot,
                                           endDepot,
                                           fixedCost,
                                           twEarly,
                                           twLate,
                                           shiftDuration,
                                           maxDistance,
                                           unitDistanceCost,
                                           unitDurationCost,
                                           profile,
                                           startLate,
                                           initialLoad,
                                           reloadDepots,
                                           maxReloads,
                                           maxOvertime,
                                           unitOvertimeCost,
                                           parsedDurationCostFunction,
                                           name,
                                           durationCostFunctionProvided);
            },
            py::arg("num_available") = py::none(),
            py::arg("capacity") = py::none(),
            py::arg("start_depot") = py::none(),
            py::arg("end_depot") = py::none(),
            py::arg("fixed_cost") = py::none(),
            py::arg("tw_early") = py::none(),
            py::arg("tw_late") = py::none(),
            py::arg("shift_duration") = py::none(),
            py::arg("max_distance") = py::none(),
            py::arg("unit_distance_cost") = py::none(),
            py::arg("unit_duration_cost") = py::none(),
            py::arg("profile") = py::none(),
            py::arg("start_late") = py::none(),
            py::arg("initial_load") = py::none(),
            py::arg("reload_depots") = py::none(),
            py::arg("max_reloads") = py::none(),
            py::arg("max_overtime") = py::none(),
            py::arg("unit_overtime_cost") = py::none(),
            // Default is ellipsis, not None, so we can distinguish omitted from
            // explicit None in the lambda above.
            py::arg("duration_cost_function") = py::ellipsis(),
            py::kw_only(),
            py::arg("name") = py::none(),
            DOC(pyvrp, ProblemData, VehicleType, replace))
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](ProblemData::VehicleType const &vehicleType) {  // __getstate__
                // NOTE:
                // We currently serialise both legacy scalar duration-cost
                // fields (unitDurationCost/unitOvertimeCost/shiftDuration) and
                // the resolved durationCostFunction. This keeps the pickle
                // payload explicit and deterministic across legacy and custom
                // modes.
                return py::make_tuple(vehicleType.numAvailable,
                                      vehicleType.capacity,
                                      vehicleType.startDepot,
                                      vehicleType.endDepot,
                                      vehicleType.fixedCost,
                                      vehicleType.twEarly,
                                      vehicleType.twLate,
                                      vehicleType.shiftDuration,
                                      vehicleType.maxDistance,
                                      vehicleType.unitDistanceCost,
                                      vehicleType.unitDurationCost,
                                      vehicleType.profile,
                                      vehicleType.startLate,
                                      vehicleType.initialLoad,
                                      vehicleType.reloadDepots,
                                      vehicleType.maxReloads,
                                      vehicleType.maxOvertime,
                                      vehicleType.unitOvertimeCost,
                                      vehicleType.durationCostFunction,
                                      vehicleType.name);
            },
            [](py::tuple t) {  // __setstate__
                // NOTE:
                // VehicleType construction is strict: custom duration function
                // and non-zero legacy duration/overtime costs are mutually
                // exclusive. But the pickle schema stores both representations.
                // During load we therefore normalise equivalent mixed state
                // (function equals legacy fromLinear form) back to legacy mode.
                // This preserves strict runtime semantics without breaking
                // round-trip deserialisation.
                auto const shiftDuration = t[7].cast<pyvrp::Duration>();
                auto const unitDurationCost = t[10].cast<pyvrp::Cost>();
                auto const unitOvertimeCost = t[17].cast<pyvrp::Cost>();
                auto const durationCostFunction
                    = t[18].cast<DurationCostFunction>();

                auto const legacyDurationCostFunction
                    = DurationCostFunction::fromLinear(
                        shiftDuration, unitDurationCost, unitOvertimeCost);

                // If the serialized function is exactly the legacy equivalent,
                // pass nullopt so VehicleType resolves from legacy scalars.
                // Otherwise keep the custom function and let strict validation
                // reject incompatible mixed inputs.
                auto const maybeDurationCostFunction
                    = durationCostFunction == legacyDurationCostFunction
                          ? std::optional<DurationCostFunction>{}
                          : std::optional<DurationCostFunction>{
                                durationCostFunction};

                ProblemData::VehicleType vehicleType(
                    t[0].cast<size_t>(),                    // num available
                    t[1].cast<std::vector<pyvrp::Load>>(),  // capacity
                    t[2].cast<size_t>(),                    // start depot
                    t[3].cast<size_t>(),                    // end depot
                    t[4].cast<pyvrp::Cost>(),               // fixed cost
                    t[5].cast<pyvrp::Duration>(),           // tw early
                    t[6].cast<pyvrp::Duration>(),           // tw late
                    shiftDuration,                          // shift duration
                    t[8].cast<pyvrp::Distance>(),           // max distance
                    t[9].cast<pyvrp::Cost>(),       // unit distance cost
                    unitDurationCost,               // unit duration cost
                    t[11].cast<size_t>(),           // profile
                    t[12].cast<pyvrp::Duration>(),  // start late
                    t[13].cast<std::vector<pyvrp::Load>>(),  // initial load
                    t[14].cast<std::vector<size_t>>(),       // reload depots
                    t[15].cast<size_t>(),                    // max reloads
                    t[16].cast<pyvrp::Duration>(),           // max overtime
                    unitOvertimeCost,                        // unit overtime
                    maybeDurationCostFunction,               // duration cost fn
                    t[19].cast<std::string>());              // name

                return vehicleType;
            }))
        .def(
            "__str__",
            [](ProblemData::VehicleType const &vehType)
            { return vehType.name; },
            py::return_value_policy::reference_internal);

    py::class_<ProblemData>(m, "ProblemData", DOC(pyvrp, ProblemData))
        .def(py::init<std::vector<ProblemData::Client>,
                      std::vector<ProblemData::Depot>,
                      std::vector<ProblemData::VehicleType>,
                      std::vector<Matrix<pyvrp::Distance>>,
                      std::vector<Matrix<pyvrp::Duration>>,
                      std::vector<ProblemData::ClientGroup>>(),
             py::arg("clients"),
             py::arg("depots"),
             py::arg("vehicle_types"),
             py::arg("distance_matrices"),
             py::arg("duration_matrices"),
             py::arg("groups") = py::list())
        .def("replace",
             &ProblemData::replace,
             py::arg("clients") = py::none(),
             py::arg("depots") = py::none(),
             py::arg("vehicle_types") = py::none(),
             py::arg("distance_matrices") = py::none(),
             py::arg("duration_matrices") = py::none(),
             py::arg("groups") = py::none(),
             DOC(pyvrp, ProblemData, replace))
        .def_property_readonly("num_clients",
                               &ProblemData::numClients,
                               DOC(pyvrp, ProblemData, numClients))
        .def_property_readonly("num_depots",
                               &ProblemData::numDepots,
                               DOC(pyvrp, ProblemData, numDepots))
        .def_property_readonly("num_groups",
                               &ProblemData::numGroups,
                               DOC(pyvrp, ProblemData, numGroups))
        .def_property_readonly("num_locations",
                               &ProblemData::numLocations,
                               DOC(pyvrp, ProblemData, numLocations))
        .def_property_readonly("num_vehicle_types",
                               &ProblemData::numVehicleTypes,
                               DOC(pyvrp, ProblemData, numVehicleTypes))
        .def_property_readonly("num_vehicles",
                               &ProblemData::numVehicles,
                               DOC(pyvrp, ProblemData, numVehicles))
        .def_property_readonly("num_profiles",
                               &ProblemData::numProfiles,
                               DOC(pyvrp, ProblemData, numProfiles))
        .def_property_readonly("num_load_dimensions",
                               &ProblemData::numLoadDimensions,
                               DOC(pyvrp, ProblemData, numLoadDimensions))
        .def(
            "location",
            [](ProblemData const &data,
               size_t idx) -> std::variant<ProblemData::Client const *,
                                           ProblemData::Depot const *>
            {
                if (idx >= data.numLocations())
                    throw py::index_error();

                auto const proxy = data.location(idx);
                if (idx < data.numDepots())
                    return proxy.depot;
                else
                    return proxy.client;
            },
            py::arg("idx"),
            py::return_value_policy::reference_internal,
            DOC(pyvrp, ProblemData, location))
        .def("clients",
             &ProblemData::clients,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, clients))
        .def("depots",
             &ProblemData::depots,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, depots))
        .def("groups",
             &ProblemData::groups,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, groups))
        .def("vehicle_types",
             &ProblemData::vehicleTypes,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, vehicleTypes))
        .def("distance_matrices",
             &ProblemData::distanceMatrices,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, distanceMatrices))
        .def("duration_matrices",
             &ProblemData::durationMatrices,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, durationMatrices))
        .def("group",
             &ProblemData::group,
             py::arg("group"),
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, group))
        .def("vehicle_type",
             &ProblemData::vehicleType,
             py::arg("vehicle_type"),
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, vehicleType))
        .def("distance_matrix",
             &ProblemData::distanceMatrix,
             py::arg("profile"),
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, distanceMatrix))
        .def("duration_matrix",
             &ProblemData::durationMatrix,
             py::arg("profile"),
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, durationMatrix))
        .def("has_time_windows",
             &ProblemData::hasTimeWindows,
             DOC(pyvrp, ProblemData, hasTimeWindows))
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](ProblemData const &data) {  // __getstate__
                return py::make_tuple(data.clients(),
                                      data.depots(),
                                      data.vehicleTypes(),
                                      data.distanceMatrices(),
                                      data.durationMatrices(),
                                      data.groups());
            },
            [](py::tuple t) {  // __setstate__
                using Clients = std::vector<ProblemData::Client>;
                using Depots = std::vector<ProblemData::Depot>;
                using VehicleTypes = std::vector<ProblemData::VehicleType>;
                using DistMats = std::vector<pyvrp::Matrix<pyvrp::Distance>>;
                using DurMats = std::vector<pyvrp::Matrix<pyvrp::Duration>>;
                using Groups = std::vector<ProblemData::ClientGroup>;

                ProblemData data(t[0].cast<Clients>(),
                                 t[1].cast<Depots>(),
                                 t[2].cast<VehicleTypes>(),
                                 t[3].cast<DistMats>(),
                                 t[4].cast<DurMats>(),
                                 t[5].cast<Groups>());

                return data;
            }));

    py::class_<Trip>(m, "Trip", DOC(pyvrp, Trip))
        .def(py::init<ProblemData const &,
                      std::vector<size_t>,
                      size_t,
                      std::optional<size_t>,
                      std::optional<size_t>>(),
             py::arg("data"),
             py::arg("visits"),
             py::arg("vehicle_type"),
             py::arg("start_depot") = py::none(),
             py::arg("end_depot") = py::none())
        .def("visits",
             &Trip::visits,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Trip, visits))
        .def("distance", &Trip::distance, DOC(pyvrp, Trip, distance))
        .def("delivery",
             &Trip::delivery,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Trip, delivery))
        .def("pickup",
             &Trip::pickup,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Trip, pickup))
        .def("load",
             &Trip::load,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Trip, load))
        .def("excess_load",
             &Trip::excessLoad,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Trip, excessLoad))
        .def("travel_duration",
             &Trip::travelDuration,
             DOC(pyvrp, Trip, travelDuration))
        .def("service_duration",
             &Trip::serviceDuration,
             DOC(pyvrp, Trip, serviceDuration))
        .def("release_time", &Trip::releaseTime, DOC(pyvrp, Trip, releaseTime))
        .def("prizes", &Trip::prizes, DOC(pyvrp, Trip, prizes))
        .def("vehicle_type", &Trip::vehicleType, DOC(pyvrp, Trip, vehicleType))
        .def("start_depot", &Trip::startDepot, DOC(pyvrp, Trip, startDepot))
        .def("end_depot", &Trip::endDepot, DOC(pyvrp, Trip, endDepot))
        .def("has_excess_load",
             &Trip::hasExcessLoad,
             DOC(pyvrp, Trip, hasExcessLoad))
        .def(py::self == py::self)  // this is __eq__
        .def("__len__", &Trip::size, DOC(pyvrp, Trip, size))
        .def(
            "__iter__",
            [](Trip const &trip)
            { return py::make_iterator(trip.begin(), trip.end()); },
            py::return_value_policy::reference_internal)
        .def(
            "__getitem__",
            [](Trip const &trip, int idx)
            {
                // int so we also support negative offsets from the end.
                idx = idx < 0 ? trip.size() + idx : idx;
                if (idx < 0 || static_cast<size_t>(idx) >= trip.size())
                    throw py::index_error();
                return trip[idx];
            },
            py::arg("idx"))
        .def(py::pickle(
            [](Trip const &trip) {  // __getstate__
                // Returns a tuple that completely encodes the trip's state.
                return py::make_tuple(trip.visits(),
                                      trip.distance(),
                                      trip.delivery(),
                                      trip.pickup(),
                                      trip.load(),
                                      trip.excessLoad(),
                                      trip.travelDuration(),
                                      trip.serviceDuration(),
                                      trip.releaseTime(),
                                      trip.prizes(),
                                      trip.vehicleType(),
                                      trip.startDepot(),
                                      trip.endDepot());
            },
            [](py::tuple t) {  // __setstate__
                using Loads = std::vector<pyvrp::Load>;

                Trip trip(t[0].cast<Trip::Visits>(),     // visits
                          t[1].cast<pyvrp::Distance>(),  // distance
                          t[2].cast<Loads>(),            // delivery
                          t[3].cast<Loads>(),            // pickup
                          t[4].cast<Loads>(),            // load
                          t[5].cast<Loads>(),            // excess load
                          t[6].cast<pyvrp::Duration>(),  // travel
                          t[7].cast<pyvrp::Duration>(),  // service
                          t[8].cast<pyvrp::Duration>(),  // release
                          t[9].cast<pyvrp::Cost>(),      // prizes
                          t[10].cast<size_t>(),          // vehicle type
                          t[11].cast<size_t>(),          // start depot
                          t[12].cast<size_t>());         // end depot

                return trip;
            }))
        .def("__str__",
             [](Trip const &trip)
             {
                 std::stringstream stream;
                 stream << trip;
                 return stream.str();
             });

    py::class_<Route::ScheduledVisit>(
        m, "ScheduledVisit", DOC(pyvrp, Route, ScheduledVisit))
        .def_readonly("location", &Route::ScheduledVisit::location)
        .def_readonly("trip", &Route::ScheduledVisit::trip)
        .def_readonly("start_service", &Route::ScheduledVisit::startService)
        .def_readonly("end_service", &Route::ScheduledVisit::endService)
        .def_readonly("wait_duration", &Route::ScheduledVisit::waitDuration)
        .def_readonly("time_warp", &Route::ScheduledVisit::timeWarp)
        .def_property_readonly("service_duration",
                               &Route::ScheduledVisit::serviceDuration)
        .def(py::pickle(
            [](Route::ScheduledVisit const &visit) {  // __getstate__
                return py::make_tuple(visit.location,
                                      visit.trip,
                                      visit.startService,
                                      visit.endService,
                                      visit.waitDuration,
                                      visit.timeWarp);
            },
            [](py::tuple t) {  // __setstate__
                Route::ScheduledVisit visit(
                    t[0].cast<size_t>(),            // location
                    t[1].cast<size_t>(),            // trip
                    t[2].cast<pyvrp::Duration>(),   // start service
                    t[3].cast<pyvrp::Duration>(),   // end service
                    t[4].cast<pyvrp::Duration>(),   // wait duration
                    t[5].cast<pyvrp::Duration>());  // time warp

                return visit;
            }));

    py::class_<Route>(m, "Route", DOC(pyvrp, Route))
        .def(py::init<ProblemData const &, std::vector<size_t>, size_t>(),
             py::arg("data"),
             py::arg("visits"),
             py::arg("vehicle_type"))
        .def(py::init<ProblemData const &, std::vector<Trip>, size_t>(),
             py::arg("data"),
             py::arg("visits"),  // name is compatible with other constructor
             py::arg("vehicle_type"))
        .def("num_trips", &Route::numTrips, DOC(pyvrp, Route, numTrips))
        .def("trips",
             &Route::trips,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Route, trips))
        .def("trip",
             &Route::trip,
             py::arg("idx"),
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Route, trip))
        .def("visits",
             &Route::visits,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Route, visits))
        .def("fixed_vehicle_cost",
             &Route::fixedVehicleCost,
             DOC(pyvrp, Route, fixedVehicleCost))
        .def("distance", &Route::distance, DOC(pyvrp, Route, distance))
        .def("distance_cost",
             &Route::distanceCost,
             DOC(pyvrp, Route, distanceCost))
        .def("excess_distance",
             &Route::excessDistance,
             DOC(pyvrp, Route, excessDistance))
        .def("delivery",
             &Route::delivery,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Route, delivery))
        .def("pickup",
             &Route::pickup,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Route, pickup))
        .def("excess_load",
             &Route::excessLoad,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Route, excessLoad))
        .def("duration", &Route::duration, DOC(pyvrp, Route, duration))
        .def("overtime", &Route::overtime, DOC(pyvrp, Route, overtime))
        .def("duration_cost",
             &Route::durationCost,
             DOC(pyvrp, Route, durationCost))
        .def("time_warp", &Route::timeWarp, DOC(pyvrp, Route, timeWarp))
        .def("start_time", &Route::startTime, DOC(pyvrp, Route, startTime))
        .def("end_time", &Route::endTime, DOC(pyvrp, Route, endTime))
        .def("slack", &Route::slack, DOC(pyvrp, Route, slack))
        .def("travel_duration",
             &Route::travelDuration,
             DOC(pyvrp, Route, travelDuration))
        .def("service_duration",
             &Route::serviceDuration,
             DOC(pyvrp, Route, serviceDuration))
        .def("wait_duration",
             &Route::waitDuration,
             DOC(pyvrp, Route, waitDuration))
        .def(
            "release_time", &Route::releaseTime, DOC(pyvrp, Route, releaseTime))
        .def("prizes", &Route::prizes, DOC(pyvrp, Route, prizes))
        .def(
            "vehicle_type", &Route::vehicleType, DOC(pyvrp, Route, vehicleType))
        .def("start_depot", &Route::startDepot, DOC(pyvrp, Route, startDepot))
        .def("end_depot", &Route::endDepot, DOC(pyvrp, Route, endDepot))
        .def("is_feasible", &Route::isFeasible, DOC(pyvrp, Route, isFeasible))
        .def("has_excess_load",
             &Route::hasExcessLoad,
             DOC(pyvrp, Route, hasExcessLoad))
        .def("has_excess_distance",
             &Route::hasExcessDistance,
             DOC(pyvrp, Route, hasExcessDistance))
        .def("has_time_warp",
             &Route::hasTimeWarp,
             DOC(pyvrp, Route, hasTimeWarp))
        .def("schedule", &Route::schedule, DOC(pyvrp, Route, schedule))
        .def("__len__", &Route::size, DOC(pyvrp, Route, size))
        .def(
            "__iter__",
            [](Route const &route)
            { return py::make_iterator(route.begin(), route.end()); },
            py::return_value_policy::reference_internal)
        .def(
            "__getitem__",
            [](Route const &route, int idx)
            {
                // conditional so we support negative offsets from the end.
                return route[idx < 0 ? route.size() + idx : idx];
            },
            py::arg("idx"))
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](Route const &route) {  // __getstate__
                // Returns a tuple that completely encodes the route's state.
                return py::make_tuple(route.trips(),
                                      route.distance(),
                                      route.distanceCost(),
                                      route.excessDistance(),
                                      route.delivery(),
                                      route.pickup(),
                                      route.excessLoad(),
                                      route.duration(),
                                      route.overtime(),
                                      route.durationCost(),
                                      route.timeWarp(),
                                      route.travelDuration(),
                                      route.serviceDuration(),
                                      route.startTime(),
                                      route.slack(),
                                      route.prizes(),
                                      route.vehicleType(),
                                      route.startDepot(),
                                      route.endDepot(),
                                      route.schedule());
            },
            [](py::tuple t) {  // __setstate__
                using Trips = std::vector<Trip>;
                using Schedule = std::vector<Route::ScheduledVisit>;

                Route route(
                    t[0].cast<Trips>(),                     // trips
                    t[1].cast<pyvrp::Distance>(),           // distance
                    t[2].cast<pyvrp::Cost>(),               // distance cost
                    t[3].cast<pyvrp::Distance>(),           // excess distance
                    t[4].cast<std::vector<pyvrp::Load>>(),  // delivery
                    t[5].cast<std::vector<pyvrp::Load>>(),  // pickup
                    t[6].cast<std::vector<pyvrp::Load>>(),  // excess load
                    t[7].cast<pyvrp::Duration>(),           // duration
                    t[8].cast<pyvrp::Duration>(),           // overtime
                    t[9].cast<pyvrp::Cost>(),               // duration cost
                    t[10].cast<pyvrp::Duration>(),          // time warp
                    t[11].cast<pyvrp::Duration>(),          // travel
                    t[12].cast<pyvrp::Duration>(),          // service
                    t[13].cast<pyvrp::Duration>(),          // start time
                    t[14].cast<pyvrp::Duration>(),          // slack
                    t[15].cast<pyvrp::Cost>(),              // prizes
                    t[16].cast<size_t>(),                   // vehicle type
                    t[17].cast<size_t>(),                   // start depot
                    t[18].cast<size_t>(),                   // end depot
                    t[19].cast<Schedule>());                // visit schedule

                return route;
            }))
        .def("__str__",
             [](Route const &route)
             {
                 std::stringstream stream;
                 stream << route;
                 return stream.str();
             });

    py::class_<Solution, std::shared_ptr<Solution>>(
        m, "Solution", DOC(pyvrp, Solution))
        // Since Route implements __len__ and __getitem__, it is convertible to
        // std::vector<size_t> and thus a list of Routes is a valid argument for
        // both constructors. We want to avoid using the second constructor
        // since that would lose the vehicle type associations. As pybind11
        // will use the first matching constructor we put this one first.
        .def(py::init<ProblemData const &, std::vector<Route>>(),
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
                    [](ProblemData const &data, RandomNumberGenerator &rng)
                    { return Solution(data, rng); },
                    py::arg("data"),
                    py::arg("rng"),
                    DOC(pyvrp, Solution, Solution));
            })
        .def(
            "num_routes", &Solution::numRoutes, DOC(pyvrp, Solution, numRoutes))
        .def("num_trips", &Solution::numTrips, DOC(pyvrp, Solution, numTrips))
        .def("num_clients",
             &Solution::numClients,
             DOC(pyvrp, Solution, numClients))
        .def("num_missing_clients",
             &Solution::numMissingClients,
             DOC(pyvrp, Solution, numMissingClients))
        .def("num_missing_groups",
             &Solution::numMissingGroups,
             DOC(pyvrp, Solution, numMissingGroups))
        .def("routes",
             &Solution::routes,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Solution, routes))
        .def("neighbours",
             &Solution::neighbours,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Solution, neighbours))
        .def("is_feasible",
             &Solution::isFeasible,
             DOC(pyvrp, Solution, isFeasible))
        .def("is_complete",
             &Solution::isComplete,
             DOC(pyvrp, Solution, isComplete))
        .def("has_excess_load",
             &Solution::hasExcessLoad,
             DOC(pyvrp, Solution, hasExcessLoad))
        .def("has_excess_distance",
             &Solution::hasExcessDistance,
             DOC(pyvrp, Solution, hasExcessDistance))
        .def("has_time_warp",
             &Solution::hasTimeWarp,
             DOC(pyvrp, Solution, hasTimeWarp))
        .def("distance", &Solution::distance, DOC(pyvrp, Solution, distance))
        .def("distance_cost",
             &Solution::distanceCost,
             DOC(pyvrp, Solution, distanceCost))
        .def("duration", &Solution::duration, DOC(pyvrp, Solution, duration))
        .def("overtime", &Solution::overtime, DOC(pyvrp, Solution, overtime))
        .def("duration_cost",
             &Solution::durationCost,
             DOC(pyvrp, Solution, durationCost))
        .def("excess_load",
             &Solution::excessLoad,
             DOC(pyvrp, Solution, excessLoad))
        .def("excess_distance",
             &Solution::excessDistance,
             DOC(pyvrp, Solution, excessDistance))
        .def("fixed_vehicle_cost",
             &Solution::fixedVehicleCost,
             DOC(pyvrp, Solution, fixedVehicleCost))
        .def("time_warp", &Solution::timeWarp, DOC(pyvrp, Solution, timeWarp))
        .def("prizes", &Solution::prizes, DOC(pyvrp, Solution, prizes))
        .def("uncollected_prizes",
             &Solution::uncollectedPrizes,
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
                                      sol.numMissingGroups(),
                                      sol.distance(),
                                      sol.distanceCost(),
                                      sol.duration(),
                                      sol.overtime(),
                                      sol.durationCost(),
                                      sol.excessDistance(),
                                      sol.excessLoad(),
                                      sol.fixedVehicleCost(),
                                      sol.prizes(),
                                      sol.uncollectedPrizes(),
                                      sol.timeWarp(),
                                      sol.routes(),
                                      sol.neighbours());
            },
            [](py::tuple t) {  // __setstate__
                using Routes = std::vector<Route>;
                using Neighbours
                    = std::vector<std::optional<std::pair<size_t, size_t>>>;

                Solution sol(
                    t[0].cast<size_t>(),                    // num clients
                    t[1].cast<size_t>(),                    // num miss clients
                    t[2].cast<size_t>(),                    // num miss groups
                    t[3].cast<pyvrp::Distance>(),           // distance
                    t[4].cast<pyvrp::Cost>(),               // distance cost
                    t[5].cast<pyvrp::Duration>(),           // duration
                    t[6].cast<pyvrp::Duration>(),           // overtime
                    t[7].cast<pyvrp::Cost>(),               // duration cost
                    t[8].cast<pyvrp::Distance>(),           // excess distance
                    t[9].cast<std::vector<pyvrp::Load>>(),  // excess load
                    t[10].cast<pyvrp::Cost>(),              // fixed veh cost
                    t[11].cast<pyvrp::Cost>(),              // prizes
                    t[12].cast<pyvrp::Cost>(),              // uncollected
                    t[13].cast<pyvrp::Duration>(),          // time warp
                    t[14].cast<Routes>(),                   // routes
                    t[15].cast<Neighbours>());              // neighbours

                return sol;
            }))
        .def("__str__",
             [](Solution const &sol)
             {
                 std::stringstream stream;
                 stream << sol;
                 return stream.str();
             });

    py::class_<CostEvaluator>(m, "CostEvaluator", DOC(pyvrp, CostEvaluator))
        .def(py::init<std::vector<double>, double, double>(),
             py::arg("load_penalties"),
             py::arg("tw_penalty"),
             py::arg("dist_penalty"))
        .def("load_penalty",
             &CostEvaluator::loadPenalty,
             py::arg("load"),
             py::arg("capacity"),
             py::arg("dimension"),
             DOC(pyvrp, CostEvaluator, loadPenalty))
        .def("tw_penalty",
             &CostEvaluator::twPenalty,
             py::arg("time_warp"),
             DOC(pyvrp, CostEvaluator, twPenalty))
        .def("dist_penalty",
             &CostEvaluator::distPenalty,
             py::arg("distance"),
             py::arg("max_distance"),
             DOC(pyvrp, CostEvaluator, distPenalty))
        .def("penalised_cost",
             &CostEvaluator::penalisedCost<Solution>,
             py::arg("solution"),
             DOC(pyvrp, CostEvaluator, penalisedCost))
        .def("cost",
             &CostEvaluator::cost<Solution>,
             py::arg("solution"),
             DOC(pyvrp, CostEvaluator, cost));

    py::class_<LoadSegment>(m, "LoadSegment", DOC(pyvrp, LoadSegment))
        .def(py::init<pyvrp::Load, pyvrp::Load, pyvrp::Load, pyvrp::Load>(),
             py::arg("delivery"),
             py::arg("pickup"),
             py::arg("load"),
             py::arg("excess_load") = 0)
        .def("delivery",
             &LoadSegment::delivery,
             DOC(pyvrp, LoadSegment, delivery))
        .def("pickup", &LoadSegment::pickup, DOC(pyvrp, LoadSegment, pickup))
        .def("load", &LoadSegment::load, DOC(pyvrp, LoadSegment, load))
        .def("excess_load",
             &LoadSegment::excessLoad,
             py::arg("capacity"),
             DOC(pyvrp, LoadSegment, excessLoad))
        .def("finalise",
             &LoadSegment::finalise,
             py::arg("capacity"),
             DOC(pyvrp, LoadSegment, finalise))
        .def_static(
            "merge", &LoadSegment::merge, py::arg("first"), py::arg("second"))
        .def("__str__",
             [](LoadSegment const &segment)
             {
                 std::stringstream stream;
                 stream << segment;
                 return stream.str();
             });

    py::class_<DurationSegment>(
        m, "DurationSegment", DOC(pyvrp, DurationSegment))
        .def(py::init<pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration>(),
             py::arg("duration") = 0,
             py::arg("time_warp") = 0,
             py::arg("start_early") = 0,
             py::arg("start_late")
             = std::numeric_limits<pyvrp::Duration>::max(),
             py::arg("release_time") = 0,
             py::arg("cum_duration") = 0,
             py::arg("cum_time_warp") = 0,
             py::arg("prev_end_late")
             = std::numeric_limits<pyvrp::Duration>::max())
        .def("duration",
             &DurationSegment::duration,
             DOC(pyvrp, DurationSegment, duration))
        .def("finalise_back",
             &DurationSegment::finaliseBack,
             DOC(pyvrp, DurationSegment, finaliseBack))
        .def("finalise_front",
             &DurationSegment::finaliseFront,
             DOC(pyvrp, DurationSegment, finaliseFront))
        .def("start_early",
             &DurationSegment::startEarly,
             DOC(pyvrp, DurationSegment, startEarly))
        .def("start_late",
             &DurationSegment::startLate,
             DOC(pyvrp, DurationSegment, startLate))
        .def("end_early",
             &DurationSegment::endEarly,
             DOC(pyvrp, DurationSegment, endEarly))
        .def("end_late",
             &DurationSegment::endLate,
             DOC(pyvrp, DurationSegment, endLate))
        .def("prev_end_late",
             &DurationSegment::prevEndLate,
             DOC(pyvrp, DurationSegment, prevEndLate))
        .def("release_time",
             &DurationSegment::releaseTime,
             DOC(pyvrp, DurationSegment, releaseTime))
        .def("slack",
             &DurationSegment::slack,
             DOC(pyvrp, DurationSegment, slack))
        .def("time_warp",
             &DurationSegment::timeWarp,
             py::arg("max_duration")
             = std::numeric_limits<pyvrp::Duration>::max(),
             DOC(pyvrp, DurationSegment, timeWarp))
        .def_static(
            "merge",
            py::overload_cast<pyvrp::Duration const,
                              DurationSegment const &,
                              DurationSegment const &>(&DurationSegment::merge),
            py::arg("edge_duration"),
            py::arg("first"),
            py::arg("second"))
        .def("__str__",
             [](DurationSegment const &segment)
             {
                 std::stringstream stream;
                 stream << segment;
                 return stream.str();
             });

    py::class_<RandomNumberGenerator>(
        m, "RandomNumberGenerator", DOC(pyvrp, RandomNumberGenerator))
        .def(py::init<uint32_t>(), py::arg("seed"))
        .def(py::init<std::array<uint32_t, 4>>(), py::arg("state"))
        .def("min", &RandomNumberGenerator::min)
        .def("max", &RandomNumberGenerator::max)
        .def("__call__", &RandomNumberGenerator::operator())
        .def("rand", &RandomNumberGenerator::rand)
        .def("randint", &RandomNumberGenerator::randint<int>, py::arg("high"))
        .def("state", &RandomNumberGenerator::state);
}
