#include "Trip.h"

using pyvrp::Trip;

Trip::Trip([[maybe_unused]] [[maybe_unused]] ProblemData const &data,
           [[maybe_unused]] Visits visits,
           TripDelimiter start,
           TripDelimiter end,
           [[maybe_unused]] Trip const *after)
    : start_(std::move(start)), end_(std::move(end))
{
}
