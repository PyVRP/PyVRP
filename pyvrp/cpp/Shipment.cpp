#include "Shipment.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

using pyvrp::Shipment;

namespace
{
// Small local helper for what is essentially strdup() from the C23 standard,
// which my compiler does not (yet) have. See here for the actual recipe:
// https://stackoverflow.com/a/252802/4316405 (modified to use new instead of
// malloc). We do all this so we can use C-style strings, rather than C++'s
// std::string, which are much larger objects.
static char *duplicate(char const *src)
{
    char *dst = new char[std::strlen(src) + 1];  // space for src + null
    std::strcpy(dst, src);
    return dst;
}

bool isNegative(auto value) { return value < 0; }
}  // namespace

Shipment::Step::Step(size_t location,
                     Duration twEarly,
                     Duration twLate,
                     Duration serviceDuration)
    : location(location),
      twEarly(twEarly),
      twLate(twLate),
      serviceDuration(serviceDuration)
{
    if (serviceDuration < 0)
        throw std::invalid_argument("service_duration must be >= 0.");

    if (twEarly > twLate)
        throw std::invalid_argument("tw_early must be <= tw_late.");

    if (twEarly < 0)
        throw std::invalid_argument("tw_early must be >= 0.");
}

Shipment::Shipment(size_t pickupLocation,
                   size_t deliveryLocation,
                   Duration pickupTwEarly,
                   Duration pickupTwLate,
                   Duration pickupServiceDuration,
                   Duration deliveryTwEarly,
                   Duration deliveryTwLate,
                   Duration deliveryServiceDuration,
                   std::vector<Load> amount,
                   Cost prize,
                   bool required,
                   std::string name)
    : pickup(
          pickupLocation, pickupTwEarly, pickupTwLate, pickupServiceDuration),
      delivery(deliveryLocation,
               deliveryTwEarly,
               deliveryTwLate,
               deliveryServiceDuration),
      amount(amount),
      prize(prize),
      required(required),
      name(duplicate(name.data()))
{
    if (std::any_of(amount.begin(), amount.end(), isNegative<Load>))
        throw std::invalid_argument("shipment amounts must be >= 0.");

    if (prize < 0)
        throw std::invalid_argument("prize must be >= 0.");
}

Shipment::Shipment(Step pickup,
                   Step delivery,
                   std::vector<Load> amount,
                   Cost prize,
                   bool required,
                   std::string name)
    : pickup(std::move(pickup)),
      delivery(std::move(delivery)),
      amount(std::move(amount)),
      prize(prize),
      required(required),
      name(duplicate(name.data()))
{
}

Shipment::Shipment(Shipment const &shipment)
    : pickup(shipment.pickup),
      delivery(shipment.delivery),
      amount(shipment.amount),
      prize(shipment.prize),
      required(shipment.required),
      name(duplicate(shipment.name))  // TODO duplicate
{
}

Shipment::Shipment(Shipment &&shipment)
    : pickup(std::move(shipment.pickup)),
      delivery(std::move(shipment.delivery)),
      amount(std::move(shipment.amount)),
      prize(shipment.prize),
      required(shipment.prize),
      name(shipment.name)  // we can steal
{
    shipment.name = nullptr;  // stolen
}

bool Shipment::operator==(Shipment const &other) const
{
    // clang-format off
    return pickup == other.pickup
        && delivery == other.delivery
        && amount == other.amount
        && prize == other.prize
        && required == other.required
        && std::strcmp(name, other.name) == 0;
    // clang-format on
}

Shipment::~Shipment() { delete[] name; }
