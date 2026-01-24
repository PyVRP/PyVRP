/*
 * This file is part of the PyVRP project (https://github.com/PyVRP/PyVRP), and
 * licensed under the terms of the MIT license.
 */

#ifndef PYVRP_MEASURE_H
#define PYVRP_MEASURE_H

#include <cassert>
#include <cmath>
#include <compare>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <type_traits>

namespace pyvrp
{
enum class MeasureType
{
    COORD,
    DIST,
    DURATION,
    COST,
    LOAD,
};

template <typename T>
concept NumberType = std::is_arithmetic_v<T>;

// Forward declaration so we can define the relevant type aliases early.
template <MeasureType Type, NumberType Value> class Measure;

// Type aliases. These are used throughout the program.
using Coordinate = Measure<MeasureType::COORD, double>;
using Cost = Measure<MeasureType::COST, int64_t>;
using Distance = Measure<MeasureType::DIST, int64_t>;
using Duration = Measure<MeasureType::DURATION, int64_t>;
using Load = Measure<MeasureType::LOAD, int64_t>;

//
//                 EVERYTHING BELOW IS AN IMPLEMENTATION DETAIL
//

/**
 * The measure class is a thin wrapper around an underlying value. The measure
 * forms a strong type that is only explicitly castable to other arithmetic or
 * measure types.
 *
 * The measure is equipped with a ``MeasureType`` that specifies what it is
 * intended to model, as well as a ``NumberType`` storage class for the
 * underlying data. The latter can be used to e.g. differentiate integral and
 * floating point measures.
 */
template <MeasureType _, NumberType Value> class Measure
{
    Value value = 0;

public:
    // Default construction initialises to 0.
    Measure() = default;

    // This constructor takes any arithmetic type (generally useful) and casts
    // it to the underlying Value.
    template <NumberType T>
    Measure(T const value) : value(static_cast<Value>(value))
    {
    }

    // Explicit conversions of the underlying value to other arithmetic types.
    template <NumberType T> explicit operator T() const
    {
        return static_cast<T>(value);
    }

    // Explicit conversions to other measures of the same storage type (we do
    // not want there to be data loss just by casting between measures).
    template <MeasureType Other> explicit operator Measure<Other, Value>() const
    {
        return value;
    }

    // Retrieves the underlying value.
    [[nodiscard]] Value get() const;

    // In-place unary operators.
    Measure &operator+=(Measure const rhs);
    Measure &operator-=(Measure const rhs);
    Measure &operator*=(Measure const rhs);
    Measure &operator/=(Measure const rhs);

    // Comparison operators.
    [[nodiscard]] bool operator==(Measure const other) const;
    [[nodiscard]] std::strong_ordering operator<=>(Measure const other) const;
};

// Retrieves the underlying value.
template <MeasureType Type, NumberType Value>
Value Measure<Type, Value>::get() const
{
    return value;
}

// In-place unary operators.
template <MeasureType Type, NumberType Value>
Measure<Type, Value> &
Measure<Type, Value>::operator+=(Measure<Type, Value> const rhs)
{
    if constexpr (std::is_integral_v<Value>)
    {
        [[maybe_unused]] Value res = 0;
        assert(!__builtin_add_overflow(this->value, rhs.value, &res));
    }

    this->value += rhs.value;
    return *this;
}

template <MeasureType Type, NumberType Value>
Measure<Type, Value> &
Measure<Type, Value>::operator-=(Measure<Type, Value> const rhs)
{
    if constexpr (std::is_integral_v<Value>)
    {
        [[maybe_unused]] Value res = 0;
        assert(!__builtin_sub_overflow(this->value, rhs.value, &res));
    }

    this->value -= rhs.value;
    return *this;
}

template <MeasureType Type, NumberType Value>
Measure<Type, Value> &
Measure<Type, Value>::operator*=(Measure<Type, Value> const rhs)
{
    if constexpr (std::is_integral_v<Value>)
    {
        [[maybe_unused]] Value res = 0;
        assert(!__builtin_mul_overflow(this->value, rhs.value, &res));
    }

    this->value *= rhs.value;
    return *this;
}

template <MeasureType Type, NumberType Value>
Measure<Type, Value> &
Measure<Type, Value>::operator/=(Measure<Type, Value> const rhs)
{
    this->value /= rhs.value;
    return *this;
}

// Comparison operators.
template <MeasureType Type, NumberType Value>
bool Measure<Type, Value>::operator==(Measure<Type, Value> const other) const
{
    return value == other.value;
}

template <MeasureType Type, NumberType Value>
std::strong_ordering
Measure<Type, Value>::operator<=>(Measure<Type, Value> const other) const
{
    return value <=> other.value;
}

// Free-standing binary operators.
template <MeasureType Type, NumberType Value>
Measure<Type, Value> operator+(Measure<Type, Value> const lhs,
                               Measure<Type, Value> const rhs)
{
    if constexpr (std::is_integral_v<Value>)
    {
        [[maybe_unused]] Value res = 0;
        assert(!__builtin_add_overflow(lhs.get(), rhs.get(), &res));
    }

    return lhs.get() + rhs.get();
}

template <MeasureType Type, NumberType Value>
Measure<Type, Value> operator+(Measure<Type, Value> const lhs)
{
    return +lhs.get();
}

template <MeasureType Type, NumberType Value>
Measure<Type, Value> operator-(Measure<Type, Value> const lhs,
                               Measure<Type, Value> const rhs)
{
    if constexpr (std::is_integral_v<Value>)
    {
        [[maybe_unused]] Value res = 0;
        assert(!__builtin_sub_overflow(lhs.get(), rhs.get(), &res));
    }

    return lhs.get() - rhs.get();
}

template <MeasureType Type, NumberType Value>
Measure<Type, Value> operator-(Measure<Type, Value> const lhs)
{
    return -lhs.get();
}

template <MeasureType Type, NumberType Value>
Measure<Type, Value> operator*(Measure<Type, Value> const lhs,
                               Measure<Type, Value> const rhs)
{
    if constexpr (std::is_integral_v<Value>)
    {
        [[maybe_unused]] Value res = 0;
        assert(!__builtin_mul_overflow(lhs.get(), rhs.get(), &res));
    }

    return lhs.get() * rhs.get();
}

template <MeasureType Type, NumberType Value>
Measure<Type, Value> operator/(Measure<Type, Value> const lhs,
                               Measure<Type, Value> const rhs)
{
    return lhs.get() / rhs.get();
}
}  // namespace pyvrp

// For printing.
template <pyvrp::MeasureType Type, pyvrp::NumberType Value>
std::ostream &operator<<(std::ostream &out,
                         pyvrp::Measure<Type, Value> const measure)
{
    return out << measure.get();
}

// Specialisations for hashing and numerical limits.

template <pyvrp::MeasureType Type, pyvrp::NumberType Value>
struct std::hash<pyvrp::Measure<Type, Value>>
{
    size_t operator()(pyvrp::Measure<Type, Value> const measure) const
    {
        return std::hash<Value>()(measure.get());
    }
};

template <pyvrp::MeasureType Type, pyvrp::NumberType Value>
class std::numeric_limits<pyvrp::Measure<Type, Value>>
{
public:
    static pyvrp::Measure<Type, Value> max()
    {
        return std::numeric_limits<Value>::max();
    }

    static pyvrp::Measure<Type, Value> min()
    {
        return std::numeric_limits<Value>::min();
    }
};

#endif  // PYVRP_MEASURE_H
