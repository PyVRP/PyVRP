#include "VehicleType.h"

#include <cstring>

using pyvrp::VehicleType;

namespace
{
// Pad vec1 with zeroes to the size of vec1 and vec2, whichever is larger.
auto &pad(auto &vec1, auto const &vec2)
{
    vec1.resize(std::max(vec1.size(), vec2.size()));
    return vec1;
}

bool isNegative(auto value) { return value < 0; }
}  // namespace

VehicleType::VehicleType(size_t numAvailable,
                         std::vector<Load> capacity,
                         size_t startDepot,
                         size_t endDepot,
                         Cost fixedCost,
                         Duration twEarly,
                         Duration twLate,
                         Duration shiftDuration,
                         Distance maxDistance,
                         Cost unitDistanceCost,
                         Cost unitDurationCost,
                         size_t profile,
                         std::optional<Duration> startLate,
                         std::vector<Load> initialLoad,
                         std::vector<size_t> reloadDepots,
                         size_t maxReloads,
                         Duration maxOvertime,
                         Cost unitOvertimeCost,
                         std::string name)
    : numAvailable(numAvailable),
      startDepot(startDepot),
      endDepot(endDepot),
      capacity(pad(capacity, initialLoad)),
      twEarly(twEarly),
      twLate(twLate),
      shiftDuration(shiftDuration),
      maxDistance(maxDistance),
      fixedCost(fixedCost),
      unitDistanceCost(unitDistanceCost),
      unitDurationCost(unitDurationCost),
      profile(profile),
      startLate(startLate.value_or(twLate)),
      initialLoad(pad(initialLoad, capacity)),
      reloadDepots(reloadDepots),
      maxReloads(maxReloads),
      maxOvertime(maxOvertime),
      unitOvertimeCost(unitOvertimeCost),
      // We need to check >= 0 here to avoid overflow. If the arguments are
      // negative the validation checks further below will raise, so it doesn't
      // matter what we set as long as we get to those checks.
      maxDuration(shiftDuration >= 0 && maxOvertime >= 0
                          && maxOvertime < std::numeric_limits<Duration>::max()
                                               - shiftDuration
                      ? shiftDuration + maxOvertime
                      : std::numeric_limits<Duration>::max()),
      name(std::strdup(name.data()))
{
    if (numAvailable == 0)
        throw std::invalid_argument("num_available must be > 0.");

    if (std::any_of(capacity.begin(), capacity.end(), isNegative<Load>))
        throw std::invalid_argument("capacity amounts must be >= 0.");

    if (twEarly > this->startLate)
        throw std::invalid_argument("tw_early must be <= start_late.");

    if (this->startLate > twLate)
        throw std::invalid_argument("start_late must be <= tw_late.");

    if (twEarly < 0)
        throw std::invalid_argument("tw_early must be >= 0.");

    if (shiftDuration < 0)
        throw std::invalid_argument("shift_duration must be >= 0.");

    if (maxDistance < 0)
        throw std::invalid_argument("max_distance must be >= 0.");

    if (fixedCost < 0)
        throw std::invalid_argument("fixed_cost must be >= 0.");

    if (unitDistanceCost < 0)
        throw std::invalid_argument("unit_distance_cost must be >= 0.");

    if (unitDurationCost < 0)
        throw std::invalid_argument("unit_duration_cost must be >= 0.");

    if (std::any_of(initialLoad.begin(), initialLoad.end(), isNegative<Load>))
        throw std::invalid_argument("initial load amounts must be >= 0.");

    for (size_t dim = 0; dim != initialLoad.size(); ++dim)
        if (initialLoad[dim] > capacity[dim])
            throw std::invalid_argument("initial load exceeds capacity.");

    if (maxOvertime < 0)
        throw std::invalid_argument("max_overtime must be >= 0.");

    if (unitOvertimeCost < 0)
        throw std::invalid_argument("unit_overtime_cost must be >= 0.");
}

VehicleType::VehicleType(VehicleType const &vehicleType)
    : numAvailable(vehicleType.numAvailable),
      startDepot(vehicleType.startDepot),
      endDepot(vehicleType.endDepot),
      capacity(vehicleType.capacity),
      twEarly(vehicleType.twEarly),
      twLate(vehicleType.twLate),
      shiftDuration(vehicleType.shiftDuration),
      maxDistance(vehicleType.maxDistance),
      fixedCost(vehicleType.fixedCost),
      unitDistanceCost(vehicleType.unitDistanceCost),
      unitDurationCost(vehicleType.unitDurationCost),
      profile(vehicleType.profile),
      startLate(vehicleType.startLate),
      initialLoad(vehicleType.initialLoad),
      reloadDepots(vehicleType.reloadDepots),
      maxReloads(vehicleType.maxReloads),
      maxOvertime(vehicleType.maxOvertime),
      unitOvertimeCost(vehicleType.unitOvertimeCost),
      maxDuration(vehicleType.maxDuration),
      name(std::strdup(vehicleType.name))
{
}

VehicleType::VehicleType(VehicleType &&vehicleType)
    : numAvailable(vehicleType.numAvailable),
      startDepot(vehicleType.startDepot),
      endDepot(vehicleType.endDepot),
      capacity(std::move(vehicleType.capacity)),
      twEarly(vehicleType.twEarly),
      twLate(vehicleType.twLate),
      shiftDuration(vehicleType.shiftDuration),
      maxDistance(vehicleType.maxDistance),
      fixedCost(vehicleType.fixedCost),
      unitDistanceCost(vehicleType.unitDistanceCost),
      unitDurationCost(vehicleType.unitDurationCost),
      profile(vehicleType.profile),
      startLate(vehicleType.startLate),
      initialLoad(std::move(vehicleType.initialLoad)),
      reloadDepots(std::move(vehicleType.reloadDepots)),
      maxReloads(vehicleType.maxReloads),
      maxOvertime(vehicleType.maxOvertime),
      unitOvertimeCost(vehicleType.unitOvertimeCost),
      maxDuration(vehicleType.maxDuration),
      name(vehicleType.name)  // we can steal
{
    vehicleType.name = nullptr;  // stolen
}

VehicleType::~VehicleType() { delete[] name; }

VehicleType
VehicleType::replace(std::optional<size_t> numAvailable,
                     std::optional<std::vector<Load>> capacity,
                     std::optional<size_t> startDepot,
                     std::optional<size_t> endDepot,
                     std::optional<Cost> fixedCost,
                     std::optional<Duration> twEarly,
                     std::optional<Duration> twLate,
                     std::optional<Duration> shiftDuration,
                     std::optional<Distance> maxDistance,
                     std::optional<Cost> unitDistanceCost,
                     std::optional<Cost> unitDurationCost,
                     std::optional<size_t> profile,
                     std::optional<Duration> startLate,
                     std::optional<std::vector<Load>> initialLoad,
                     std::optional<std::vector<size_t>> reloadDepots,
                     std::optional<size_t> maxReloads,
                     std::optional<Duration> maxOvertime,
                     std::optional<Cost> unitOvertimeCost,
                     std::optional<std::string> name) const
{
    return {numAvailable.value_or(this->numAvailable),
            capacity.value_or(this->capacity),
            startDepot.value_or(this->startDepot),
            endDepot.value_or(this->endDepot),
            fixedCost.value_or(this->fixedCost),
            twEarly.value_or(this->twEarly),
            twLate.value_or(this->twLate),
            shiftDuration.value_or(this->shiftDuration),
            maxDistance.value_or(this->maxDistance),
            unitDistanceCost.value_or(this->unitDistanceCost),
            unitDurationCost.value_or(this->unitDurationCost),
            profile.value_or(this->profile),
            startLate.value_or(this->startLate),
            initialLoad.value_or(this->initialLoad),
            reloadDepots.value_or(this->reloadDepots),
            maxReloads.value_or(this->maxReloads),
            maxOvertime.value_or(this->maxOvertime),
            unitOvertimeCost.value_or(this->unitOvertimeCost),
            name.value_or(this->name)};
}

size_t VehicleType::maxTrips() const
{
    // When maxReloads is at its maximum size, maxReloads + 1 wraps around to 0,
    // and then std::max() ensures we still return a reasonable value.
    return reloadDepots.empty() ? 1 : std::max(maxReloads, maxReloads + 1);
}

bool VehicleType::operator==(VehicleType const &other) const
{
    // clang-format off
    return numAvailable == other.numAvailable
        && capacity == other.capacity
        && startDepot == other.startDepot
        && endDepot == other.endDepot
        && fixedCost == other.fixedCost
        && twEarly == other.twEarly
        && twLate == other.twLate
        && shiftDuration == other.shiftDuration
        && maxDistance == other.maxDistance
        && unitDistanceCost == other.unitDistanceCost
        && unitDurationCost == other.unitDurationCost
        && profile == other.profile
        && startLate == other.startLate
        && initialLoad == other.initialLoad
        && reloadDepots == other.reloadDepots
        && maxReloads == other.maxReloads
        && maxOvertime == other.maxOvertime
        && unitOvertimeCost == other.unitOvertimeCost
        && std::strcmp(name, other.name) == 0;
    // clang-format on
}
