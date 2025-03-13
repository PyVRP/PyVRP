#include "bindings.h"
#include "CostEvaluator.h"
#include "DistanceSegment.h"
#include "DurationSegment.h"
#include "DynamicBitset.h"
#include "LoadSegment.h"
#include "Matrix.h"
#include "ProblemData.h"
#include "RandomNumberGenerator.h"
#include "Route.h"
#include "Solution.h"
#include "SubPopulation.h"
#include "pyvrp_docs.h"

#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>
#include <string>
#include <variant>

namespace py = pybind11;

using pyvrp::CostEvaluator;
using pyvrp::DistanceSegment;
using pyvrp::DurationSegment;
using pyvrp::DynamicBitset;
using pyvrp::LoadSegment;
using pyvrp::Matrix;
using pyvrp::PopulationParams;
using pyvrp::ProblemData;
using pyvrp::RandomNumberGenerator;
using pyvrp::Route;
using pyvrp::Solution;
using pyvrp::SubPopulation;

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
        .def("reset", &DynamicBitset::reset)
        .def(
            "__getitem__",
            [](DynamicBitset const &bitset, size_t idx) { return bitset[idx]; },
            py::arg("idx"))
        .def(
            "__setitem__",
            [](DynamicBitset &bitset, size_t idx, bool value)
            { bitset[idx] = value; },
            py::arg("idx"),
            py::arg("value"))
        .def("__or__", &DynamicBitset::operator|, py::arg("other"))
        .def("__and__", &DynamicBitset::operator&, py::arg("other"))
        .def("__xor__", &DynamicBitset::operator^, py::arg("other"))
        .def("__invert__", &DynamicBitset::operator~);

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
        .def_readonly("delivery", &ProblemData::Client::delivery)
        .def_readonly("pickup", &ProblemData::Client::pickup)
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
             py::arg("service_duration") = 0,
             py::arg("tw_early") = 0,
             py::arg("tw_late") = std::numeric_limits<pyvrp::Duration>::max(),
             py::kw_only(),
             py::arg("name") = "")
        .def_readonly("x", &ProblemData::Depot::x)
        .def_readonly("y", &ProblemData::Depot::y)
        .def_readonly("service_duration", &ProblemData::Depot::serviceDuration)
        .def_readonly("tw_early", &ProblemData::Depot::twEarly)
        .def_readonly("tw_late", &ProblemData::Depot::twLate)
        .def_readonly("name",
                      &ProblemData::Depot::name,
                      py::return_value_policy::reference_internal)
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](ProblemData::Depot const &depot) {  // __getstate__
                return py::make_tuple(depot.x,
                                      depot.y,
                                      depot.serviceDuration,
                                      depot.twEarly,
                                      depot.twLate,
                                      depot.name);
            },
            [](py::tuple t) {  // __setstate__
                ProblemData::Depot depot(
                    t[0].cast<pyvrp::Coordinate>(),  // x
                    t[1].cast<pyvrp::Coordinate>(),  // y
                    t[2].cast<pyvrp::Duration>(),    // service duration
                    t[3].cast<pyvrp::Duration>(),    // tw early
                    t[4].cast<pyvrp::Duration>(),    // tw late
                    t[5].cast<std::string>());       // name

                return depot;
            }))
        .def(
            "__str__",
            [](ProblemData::Depot const &depot) { return depot.name; },
            py::return_value_policy::reference_internal);

    py::class_<ProblemData::ClientGroup>(
        m, "ClientGroup", DOC(pyvrp, ProblemData, ClientGroup))
        .def(py::init<std::vector<size_t>, bool>(),
             py::arg("clients") = py::list(),
             py::arg("required") = true)
        .def("add_client",
             &ProblemData::ClientGroup::addClient,
             py::arg("client"))
        .def("clear", &ProblemData::ClientGroup::clear)
        .def_property_readonly("clients", &ProblemData::ClientGroup::clients)
        .def_readonly("required", &ProblemData::ClientGroup::required)
        .def_readonly("mutually_exclusive",
                      &ProblemData::ClientGroup::mutuallyExclusive)
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](ProblemData::ClientGroup const &group) {  // __getstate__
                return py::make_tuple(group.clients(), group.required);
            },
            [](py::tuple t) {  // __setstate__
                ProblemData::ClientGroup group(
                    t[0].cast<std::vector<size_t>>(),  // clients
                    t[1].cast<bool>());                // required

                return group;
            }))
        .def("__len__", &ProblemData::ClientGroup::size)
        .def(
            "__iter__",
            [](ProblemData::ClientGroup const &group)
            { return py::make_iterator(group.begin(), group.end()); },
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
                      char const *>(),
             py::arg("num_available") = 1,
             py::arg("capacity") = py::list(),
             py::arg("start_depot") = 0,
             py::arg("end_depot") = 0,
             py::arg("fixed_cost") = 0,
             py::arg("tw_early") = 0,
             py::arg("tw_late") = std::numeric_limits<pyvrp::Duration>::max(),
             py::arg("max_duration")
             = std::numeric_limits<pyvrp::Duration>::max(),
             py::arg("max_distance")
             = std::numeric_limits<pyvrp::Distance>::max(),
             py::arg("unit_distance_cost") = 1,
             py::arg("unit_duration_cost") = 0,
             py::arg("profile") = 0,
             py::arg("start_late") = py::none(),
             py::arg("initial_load") = py::list(),
             py::kw_only(),
             py::arg("name") = "")
        .def_readonly("num_available", &ProblemData::VehicleType::numAvailable)
        .def_readonly("capacity", &ProblemData::VehicleType::capacity)
        .def_readonly("start_depot", &ProblemData::VehicleType::startDepot)
        .def_readonly("end_depot", &ProblemData::VehicleType::endDepot)
        .def_readonly("fixed_cost", &ProblemData::VehicleType::fixedCost)
        .def_readonly("tw_early", &ProblemData::VehicleType::twEarly)
        .def_readonly("tw_late", &ProblemData::VehicleType::twLate)
        .def_readonly("max_duration", &ProblemData::VehicleType::maxDuration)
        .def_readonly("max_distance", &ProblemData::VehicleType::maxDistance)
        .def_readonly("unit_distance_cost",
                      &ProblemData::VehicleType::unitDistanceCost)
        .def_readonly("unit_duration_cost",
                      &ProblemData::VehicleType::unitDurationCost)
        .def_readonly("profile", &ProblemData::VehicleType::profile)
        .def_readonly("start_late", &ProblemData::VehicleType::startLate)
        .def_readonly("initial_load", &ProblemData::VehicleType::initialLoad)
        .def_readonly("name",
                      &ProblemData::VehicleType::name,
                      py::return_value_policy::reference_internal)
        .def("replace",
             &ProblemData::VehicleType::replace,
             py::arg("num_available") = py::none(),
             py::arg("capacity") = py::none(),
             py::arg("start_depot") = py::none(),
             py::arg("end_depot") = py::none(),
             py::arg("fixed_cost") = py::none(),
             py::arg("tw_early") = py::none(),
             py::arg("tw_late") = py::none(),
             py::arg("max_duration") = py::none(),
             py::arg("max_distance") = py::none(),
             py::arg("unit_distance_cost") = py::none(),
             py::arg("unit_duration_cost") = py::none(),
             py::arg("profile") = py::none(),
             py::arg("start_late") = py::none(),
             py::arg("initial_load") = py::none(),
             py::kw_only(),
             py::arg("name") = py::none(),
             DOC(pyvrp, ProblemData, VehicleType, replace))
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](ProblemData::VehicleType const &vehicleType) {  // __getstate__
                return py::make_tuple(vehicleType.numAvailable,
                                      vehicleType.capacity,
                                      vehicleType.startDepot,
                                      vehicleType.endDepot,
                                      vehicleType.fixedCost,
                                      vehicleType.twEarly,
                                      vehicleType.twLate,
                                      vehicleType.maxDuration,
                                      vehicleType.maxDistance,
                                      vehicleType.unitDistanceCost,
                                      vehicleType.unitDurationCost,
                                      vehicleType.profile,
                                      vehicleType.startLate,
                                      vehicleType.initialLoad,
                                      vehicleType.name);
            },
            [](py::tuple t) {  // __setstate__
                ProblemData::VehicleType vehicleType(
                    t[0].cast<size_t>(),                    // num available
                    t[1].cast<std::vector<pyvrp::Load>>(),  // capacity
                    t[2].cast<size_t>(),                    // start depot
                    t[3].cast<size_t>(),                    // end depot
                    t[4].cast<pyvrp::Cost>(),               // fixed cost
                    t[5].cast<pyvrp::Duration>(),           // tw early
                    t[6].cast<pyvrp::Duration>(),           // tw late
                    t[7].cast<pyvrp::Duration>(),           // max duration
                    t[8].cast<pyvrp::Distance>(),           // max distance
                    t[9].cast<pyvrp::Cost>(),       // unit distance cost
                    t[10].cast<pyvrp::Cost>(),      // unit duration cost
                    t[11].cast<size_t>(),           // profile
                    t[12].cast<pyvrp::Duration>(),  // start late
                    t[13].cast<std::vector<pyvrp::Load>>(),  // initial load
                    t[14].cast<std::string>());              // name

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
        .def("centroid",
             &ProblemData::centroid,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, ProblemData, centroid))
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

    py::class_<Route::ScheduledVisit>(
        m, "ScheduledVisit", DOC(pyvrp, Route, ScheduledVisit))
        .def_readonly("start_service", &Route::ScheduledVisit::startService)
        .def_readonly("end_service", &Route::ScheduledVisit::endService)
        .def_readonly("wait_duration", &Route::ScheduledVisit::waitDuration)
        .def_readonly("time_warp", &Route::ScheduledVisit::timeWarp)
        .def_property_readonly("service_duration",
                               &Route::ScheduledVisit::serviceDuration)
        .def(py::pickle(
            [](Route::ScheduledVisit const &visit) {  // __getstate__
                return py::make_tuple(visit.startService,
                                      visit.endService,
                                      visit.waitDuration,
                                      visit.timeWarp);
            },
            [](py::tuple t) {  // __setstate__
                Route::ScheduledVisit visit(
                    t[0].cast<pyvrp::Duration>(),   // start service
                    t[1].cast<pyvrp::Duration>(),   // end service
                    t[2].cast<pyvrp::Duration>(),   // wait duration
                    t[3].cast<pyvrp::Duration>());  // time warp

                return visit;
            }));

    py::class_<Route>(m, "Route", DOC(pyvrp, Route))
        .def(py::init<ProblemData const &, std::vector<size_t>, size_t>(),
             py::arg("data"),
             py::arg("visits"),
             py::arg("vehicle_type"))
        .def("visits",
             &Route::visits,
             py::return_value_policy::reference_internal,
             DOC(pyvrp, Route, visits))
        .def("distance", &Route::distance, DOC(pyvrp, Route, distance))
        .def("distance_cost",
             &Route::distanceCost,
             DOC(pyvrp, Route, distanceCost))
        .def("excess_distance",
             &Route::excessDistance,
             DOC(pyvrp, Route, excessDistance))
        .def("delivery", &Route::delivery, DOC(pyvrp, Route, delivery))
        .def("pickup", &Route::pickup, DOC(pyvrp, Route, pickup))
        .def("excess_load", &Route::excessLoad, DOC(pyvrp, Route, excessLoad))
        .def("duration", &Route::duration, DOC(pyvrp, Route, duration))
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
        .def("centroid", &Route::centroid, DOC(pyvrp, Route, centroid))
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
                // int so we also support negative offsets from the end.
                idx = idx < 0 ? route.size() + idx : idx;
                if (idx < 0 || static_cast<size_t>(idx) >= route.size())
                    throw py::index_error();
                return route[idx];
            },
            py::arg("idx"))
        .def(py::self == py::self)  // this is __eq__
        .def(py::pickle(
            [](Route const &route) {  // __getstate__
                // Returns a tuple that completely encodes the route's state.
                return py::make_tuple(route.visits(),
                                      route.distance(),
                                      route.distanceCost(),
                                      route.excessDistance(),
                                      route.delivery(),
                                      route.pickup(),
                                      route.excessLoad(),
                                      route.duration(),
                                      route.durationCost(),
                                      route.timeWarp(),
                                      route.travelDuration(),
                                      route.serviceDuration(),
                                      route.waitDuration(),
                                      route.releaseTime(),
                                      route.startTime(),
                                      route.slack(),
                                      route.prizes(),
                                      route.centroid(),
                                      route.vehicleType(),
                                      route.startDepot(),
                                      route.endDepot(),
                                      route.schedule());
            },
            [](py::tuple t) {  // __setstate__
                using Schedule = std::vector<Route::ScheduledVisit>;

                Route route(
                    t[0].cast<std::vector<size_t>>(),         // visits
                    t[1].cast<pyvrp::Distance>(),             // distance
                    t[2].cast<pyvrp::Cost>(),                 // distance cost
                    t[3].cast<pyvrp::Distance>(),             // excess distance
                    t[4].cast<std::vector<pyvrp::Load>>(),    // delivery
                    t[5].cast<std::vector<pyvrp::Load>>(),    // pickup
                    t[6].cast<std::vector<pyvrp::Load>>(),    // excess load
                    t[7].cast<pyvrp::Duration>(),             // duration
                    t[8].cast<pyvrp::Cost>(),                 // duration cost
                    t[9].cast<pyvrp::Duration>(),             // time warp
                    t[10].cast<pyvrp::Duration>(),            // travel
                    t[11].cast<pyvrp::Duration>(),            // service
                    t[12].cast<pyvrp::Duration>(),            // wait
                    t[13].cast<pyvrp::Duration>(),            // release
                    t[14].cast<pyvrp::Duration>(),            // start time
                    t[15].cast<pyvrp::Duration>(),            // slack
                    t[16].cast<pyvrp::Cost>(),                // prizes
                    t[17].cast<std::pair<double, double>>(),  // centroid
                    t[18].cast<size_t>(),                     // vehicle type
                    t[19].cast<size_t>(),                     // start depot
                    t[20].cast<size_t>(),                     // end depot
                    t[21].cast<Schedule>());                  // visit schedule

                return route;
            }))
        .def("__str__",
             [](Route const &route)
             {
                 std::stringstream stream;
                 stream << route;
                 return stream.str();
             });

    py::class_<Solution>(m, "Solution", DOC(pyvrp, Solution))
        // Since Route implements __len__ and __getitem__, it is convertible to
        // std::vector<size_t> and thus a list of Routes is a valid argument for
        // both constructors. We want to avoid using the second constructor
        // since that would lose the vehicle type associations. As pybind11
        // will use the first matching constructor we put this one first.
        .def(py::init<ProblemData const &, std::vector<Route> const &>(),
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
        .def("num_clients",
             &Solution::numClients,
             DOC(pyvrp, Solution, numClients))
        .def("num_missing_clients",
             &Solution::numMissingClients,
             DOC(pyvrp, Solution, numMissingClients))
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
        .def("is_group_feasible",
             &Solution::isGroupFeasible,
             DOC(pyvrp, Solution, isGroupFeasible))
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
                                      sol.distance(),
                                      sol.distanceCost(),
                                      sol.duration(),
                                      sol.durationCost(),
                                      sol.excessDistance(),
                                      sol.excessLoad(),
                                      sol.fixedVehicleCost(),
                                      sol.prizes(),
                                      sol.uncollectedPrizes(),
                                      sol.timeWarp(),
                                      sol.isGroupFeasible(),
                                      sol.routes(),
                                      sol.neighbours());
            },
            [](py::tuple t) {  // __setstate__
                using Routes = std::vector<Route>;
                using Neighbours
                    = std::vector<std::optional<std::pair<size_t, size_t>>>;

                Solution sol(
                    t[0].cast<size_t>(),                    // num clients
                    t[1].cast<size_t>(),                    // num missing
                    t[2].cast<pyvrp::Distance>(),           // distance
                    t[3].cast<pyvrp::Cost>(),               // distance cost
                    t[4].cast<pyvrp::Duration>(),           // duration
                    t[5].cast<pyvrp::Cost>(),               // duration cost
                    t[6].cast<pyvrp::Distance>(),           // excess distance
                    t[7].cast<std::vector<pyvrp::Load>>(),  // excess load
                    t[8].cast<pyvrp::Cost>(),               // fixed veh cost
                    t[9].cast<pyvrp::Cost>(),               // prizes
                    t[10].cast<pyvrp::Cost>(),              // uncollected
                    t[11].cast<pyvrp::Duration>(),          // time warp
                    t[12].cast<bool>(),                     // is group feasible
                    t[13].cast<Routes>(),                   // routes
                    t[14].cast<Neighbours>());              // neighbours

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
             DOC(pyvrp, CostEvaluator, twPenalty))
        .def("penalised_cost",
             &CostEvaluator::penalisedCost<Solution>,
             py::arg("solution"),
             DOC(pyvrp, CostEvaluator, penalisedCost))
        .def("cost",
             &CostEvaluator::cost<Solution>,
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
        .def(py::self == py::self, py::arg("other"))  // this is __eq__
        .def_readonly("min_pop_size", &PopulationParams::minPopSize)
        .def_readonly("generation_size", &PopulationParams::generationSize)
        .def_property_readonly("max_pop_size",
                               &PopulationParams::maxPopSize,
                               DOC(pyvrp, PopulationParams, maxPopSize))
        .def_readonly("nb_elite", &PopulationParams::nbElite)
        .def_readonly("nb_close", &PopulationParams::nbClose)
        .def_readonly("lb_diversity", &PopulationParams::lbDiversity)
        .def_readonly("ub_diversity", &PopulationParams::ubDiversity);

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
            [](SubPopulation const &subPop, int idx)
            {
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
            [](SubPopulation const &subPop)
            { return py::make_iterator(subPop.cbegin(), subPop.cend()); },
            py::return_value_policy::reference_internal)
        .def("purge",
             &SubPopulation::purge,
             py::arg("cost_evaluator"),
             DOC(pyvrp, SubPopulation, purge))
        .def("update_fitness",
             &SubPopulation::updateFitness,
             py::arg("cost_evaluator"),
             DOC(pyvrp, SubPopulation, updateFitness));

    py::class_<DistanceSegment>(
        m, "DistanceSegment", DOC(pyvrp, DistanceSegment))
        .def(py::init<pyvrp::Distance>(), py::arg("distance"))
        .def("distance",
             &DistanceSegment::distance,
             DOC(pyvrp, DistanceSegment, distance))
        .def_static("merge",
                    &DistanceSegment::merge,
                    py::arg("edge_distance"),
                    py::arg("first"),
                    py::arg("second"));

    py::class_<LoadSegment>(m, "LoadSegment", DOC(pyvrp, LoadSegment))
        .def(py::init<pyvrp::Load, pyvrp::Load, pyvrp::Load>(),
             py::arg("delivery"),
             py::arg("pickup"),
             py::arg("load"))
        .def("delivery",
             &LoadSegment::delivery,
             DOC(pyvrp, LoadSegment, delivery))
        .def("pickup", &LoadSegment::pickup, DOC(pyvrp, LoadSegment, pickup))
        .def("load", &LoadSegment::load, DOC(pyvrp, LoadSegment, load))
        .def_static(
            "merge", &LoadSegment::merge, py::arg("first"), py::arg("second"));

    py::class_<DurationSegment>(
        m, "DurationSegment", DOC(pyvrp, DurationSegment))
        .def(py::init<pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration,
                      pyvrp::Duration>(),
             py::arg("duration"),
             py::arg("time_warp"),
             py::arg("tw_early"),
             py::arg("tw_late"),
             py::arg("release_time"))
        .def("duration",
             &DurationSegment::duration,
             DOC(pyvrp, DurationSegment, duration))
        .def("tw_early",
             &DurationSegment::twEarly,
             DOC(pyvrp, DurationSegment, twEarly))
        .def("tw_late",
             &DurationSegment::twLate,
             DOC(pyvrp, DurationSegment, twLate))
        .def("time_warp",
             &DurationSegment::timeWarp,
             py::arg("max_duration")
             = std::numeric_limits<pyvrp::Duration>::max(),
             DOC(pyvrp, DurationSegment, timeWarp))
        .def_static("merge",
                    &DurationSegment::merge,
                    py::arg("edge_duration"),
                    py::arg("first"),
                    py::arg("second"));

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
