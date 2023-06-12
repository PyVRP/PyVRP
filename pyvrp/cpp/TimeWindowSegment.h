#ifndef PYVRP_TIMEWINDOWSEGMENT_H
#define PYVRP_TIMEWINDOWSEGMENT_H

#include "Matrix.h"
#include "Measure.h"
#include "ProblemData.h"

class TimeWindowSegment
{
    using TWS = TimeWindowSegment;

    size_t idxFirst_ = 0;    // Index of the first client in the segment
    size_t idxLast_ = 0;     // Index of the last client in the segment
    Duration duration_ = 0;  // Duration = travel + wait + service time
    Duration timeWarp_ = 0;  // Cumulative time warp (end - start = dur - tw)
    Duration twEarly_ = 0;   // Earliest moment to start service of first client
    Duration twLate_ = 0;    // Latest moment to start service of last client

    [[nodiscard]] inline TWS merge(Matrix<Duration> const &durationMatrix,
                                   TWS const &other) const;

public:
    template <typename... Args>
    [[nodiscard]] inline static TWS
    merge(Matrix<Duration> const &durationMatrix,
          TWS const &first,
          TWS const &second,
          Args... args);

    [[nodiscard]] inline Duration duration() const;
    [[nodiscard]] inline Duration timeWarp() const;
    [[nodiscard]] inline Duration twEarly() const;
    [[nodiscard]] inline Duration twLate() const;

    TimeWindowSegment() = default;  // TODO at least require client index

    inline TimeWindowSegment(size_t idxFirst_,
                             size_t idxLast_,
                             Duration duration,
                             Duration timeWarp,
                             Duration twEarly,
                             Duration twLate);

    inline TimeWindowSegment(size_t idx, ProblemData::Client client);
};

TimeWindowSegment TimeWindowSegment::merge(
    [[maybe_unused]] Matrix<Duration> const &durationMatrix,
    [[maybe_unused]] TimeWindowSegment const &other) const
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return {};
#else
    auto const travelDuration = durationMatrix(idxLast_, other.idxFirst_);
    auto const diff = duration_ - timeWarp_ + travelDuration;
    auto const diffWait
        = std::max<Duration>(other.twEarly_ - diff - twLate_, 0);
    auto const diffTw = std::max<Duration>(twEarly_ + diff - other.twLate_, 0);

    return {idxFirst_,
            other.idxLast_,
            duration_ + other.duration_ + travelDuration + diffWait,
            timeWarp_ + other.timeWarp_ + diffTw,
            std::max(other.twEarly_ - diff, twEarly_) - diffWait,
            std::min(other.twLate_ - diff, twLate_) + diffTw};
#endif
}

template <typename... Args>
TimeWindowSegment TimeWindowSegment::merge(
    [[maybe_unused]] Matrix<Duration> const &durationMatrix,
    [[maybe_unused]] TimeWindowSegment const &first,
    [[maybe_unused]] TimeWindowSegment const &second,
    [[maybe_unused]] Args... args)
{
#ifdef PYVRP_NO_TIME_WINDOWS
    return {};
#else
    auto const res = first.merge(durationMatrix, second);

    if constexpr (sizeof...(args) == 0)
        return res;
    else
        return merge(durationMatrix, res, args...);
#endif
}

Duration TimeWindowSegment::duration() const { return duration_; }

Duration TimeWindowSegment::timeWarp() const { return timeWarp_; }

Duration TimeWindowSegment::twEarly() const { return twEarly_; }

Duration TimeWindowSegment::twLate() const { return twLate_; }

TimeWindowSegment::TimeWindowSegment(size_t idxFirst_,
                                     size_t idxLast_,
                                     Duration duration,
                                     Duration timeWarp,
                                     Duration twEarly,
                                     Duration twLate)
    : idxFirst_(idxFirst_),
      idxLast_(idxLast_),
      duration_(duration),
      timeWarp_(timeWarp),
      twEarly_(twEarly),
      twLate_(twLate)
{
}

TimeWindowSegment::TimeWindowSegment(size_t idx, ProblemData::Client client)
    : idxFirst_(idx),
      idxLast_(idx),
      duration_(client.serviceDuration),
      timeWarp_(0),
      twEarly_(client.twEarly),
      twLate_(client.twLate)
{
}

#endif  // PYVRP_TIMEWINDOWSEGMENT_H
