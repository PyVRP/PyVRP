#include "Shipment.h"

#include <stdexcept>

using pyvrp::Shipment;

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
