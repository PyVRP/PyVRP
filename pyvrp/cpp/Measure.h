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
using Value = int64_t;

enum class MeasureType
{
    COORD,
    DIST,
    DURATION,
    COST,
    LOAD,
    INDEX,
};

// Forward declaration so we can define the relevant type aliases early.
template <MeasureType _, typename ValueType = Value> class Measure;

// Type aliases. These are used throughout the program.
using Coordinate = Measure<MeasureType::COORD>;
using Cost = Measure<MeasureType::COST>;
using Distance = Measure<MeasureType::DIST>;
using Duration = Measure<MeasureType::DURATION>;
using Load = Measure<MeasureType::LOAD>;
using Index = Measure<MeasureType::INDEX, uint32_t>;

//
//                 EVERYTHING BELOW THIS IS IMPLEMENTATION
//

/**
 * The measure class is a thin wrapper around an underlying ``ValueType``. The
 * measure forms a strong type that is only explicitly convertible into other
 * arithmetic or measure types.
 */
template <MeasureType _, typename ValueType> class Measure
{
    ValueType value;

public:
    // Default construction initialises to 0.
    Measure() = default;

    // This constructor takes any arithmetic type (generally useful) and casts
    // its value to ValueType.
    template <typename T,
              std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    Measure(T const value) : value(static_cast<ValueType>(value))
    {
    }

    // Explicit conversions of the underlying value to other arithmetic types.
    template <typename T,
              std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    explicit operator T() const
    {
        return static_cast<T>(value);
    }

    // Explicit conversions to other measures.
    template <MeasureType Other> explicit operator Measure<Other>() const
    {
        return value;
    }

    // Retreives the underlying value.
    [[nodiscard]] ValueType get() const;

    // In-place unary operators.
    Measure &operator+=(Measure const &rhs);
    Measure &operator-=(Measure const &rhs);
    Measure &operator*=(Measure const &rhs);
    Measure &operator/=(Measure const &rhs);

    // Comparison operators.
    [[nodiscard]] bool operator==(Measure const &other) const;
    [[nodiscard]] std::strong_ordering operator<=>(Measure const &other) const;
};

// Retreives the underlying value.
template <MeasureType Type, typename ValueType>
ValueType Measure<Type, ValueType>::get() const
{
    return value;
}

// In-place unary operators.
template <MeasureType Type, typename ValueType>
Measure<Type, ValueType> &
Measure<Type, ValueType>::operator+=(Measure<Type, ValueType> const &rhs)
{
    if constexpr (std::is_signed_v<ValueType>)
    {
        [[maybe_unused]] ValueType res = 0;
        assert(!__builtin_add_overflow(this->value, rhs.value, &res));
    }

    this->value += rhs.value;
    return *this;
}

template <MeasureType Type, typename ValueType>
Measure<Type, ValueType> &
Measure<Type, ValueType>::operator-=(Measure<Type, ValueType> const &rhs)
{
    if constexpr (std::is_signed_v<ValueType>)
    {
        [[maybe_unused]] ValueType res = 0;
        assert(!__builtin_sub_overflow(this->value, rhs.value, &res));
    }

    this->value -= rhs.value;
    return *this;
}

template <MeasureType Type, typename ValueType>
Measure<Type, ValueType> &
Measure<Type, ValueType>::operator*=(Measure<Type, ValueType> const &rhs)
{
    if constexpr (std::is_signed_v<ValueType>)
    {
        [[maybe_unused]] ValueType res = 0;
        assert(!__builtin_mul_overflow(this->value, rhs.value, &res));
    }

    this->value *= rhs.value;
    return *this;
}

template <MeasureType Type, typename ValueType>
Measure<Type, ValueType> &
Measure<Type, ValueType>::operator/=(Measure<Type, ValueType> const &rhs)
{
    this->value /= rhs.value;
    return *this;
}

// Comparison operators.
template <MeasureType Type, typename ValueType>
bool Measure<Type, ValueType>::operator==(
    Measure<Type, ValueType> const &other) const
{
    return value == other.value;
}

template <MeasureType Type, typename ValueType>
std::strong_ordering Measure<Type, ValueType>::operator<=>(
    Measure<Type, ValueType> const &other) const
{
    return value <=> other.value;
}

// Free-standing binary operators.
template <MeasureType Type, typename ValueType>
Measure<Type, ValueType> operator+(Measure<Type, ValueType> const lhs,
                                   Measure<Type, ValueType> const rhs)
{
    if constexpr (std::is_signed_v<ValueType>)
    {
        [[maybe_unused]] ValueType res = 0;
        assert(!__builtin_add_overflow(lhs.get(), rhs.get(), &res));
    }

    return lhs.get() + rhs.get();
}

template <MeasureType Type, typename ValueType>
Measure<Type> operator+(Measure<Type> const lhs)
{
    return +lhs.get();
}

template <MeasureType Type, typename ValueType>
Measure<Type, ValueType> operator-(Measure<Type, ValueType> const lhs,
                                   Measure<Type, ValueType> const rhs)
{
    if constexpr (std::is_signed_v<ValueType>)
    {
        [[maybe_unused]] ValueType res = 0;
        assert(!__builtin_sub_overflow(lhs.get(), rhs.get(), &res));
    }

    return lhs.get() - rhs.get();
}

template <MeasureType Type, typename ValueType>
Measure<Type, ValueType> operator-(Measure<Type, ValueType> const lhs)
{
    return -lhs.get();
}

template <MeasureType Type, typename ValueType>
Measure<Type, ValueType> operator*(Measure<Type, ValueType> const lhs,
                                   Measure<Type, ValueType> const rhs)
{
    if constexpr (std::is_signed_v<ValueType>)
    {
        [[maybe_unused]] ValueType res = 0;
        assert(!__builtin_mul_overflow(lhs.get(), rhs.get(), &res));
    }

    return lhs.get() * rhs.get();
}

template <MeasureType Type, typename ValueType>
Measure<Type, ValueType> operator/(Measure<Type, ValueType> const lhs,
                                   Measure<Type, ValueType> const rhs)
{
    return lhs.get() / rhs.get();
}
}  // namespace pyvrp

// For printing.
template <pyvrp::MeasureType Type, typename ValueType>
std::ostream &operator<<(std::ostream &out,
                         pyvrp::Measure<Type, ValueType> const measure)
{
    return out << measure.get();
}

// Specialisations for hashing and numerical limits.

template <pyvrp::MeasureType Type, typename ValueType>
struct std::hash<pyvrp::Measure<Type, ValueType>>
{
    size_t operator()(pyvrp::Measure<Type, ValueType> const measure) const
    {
        return std::hash<ValueType>()(measure.get());
    }
};

template <pyvrp::MeasureType Type, typename ValueType>
class std::numeric_limits<pyvrp::Measure<Type, ValueType>>
{
public:
    static pyvrp::Measure<Type, ValueType> max()
    {
        return std::numeric_limits<ValueType>::max();
    }

    static pyvrp::Measure<Type, ValueType> min()
    {
        return std::numeric_limits<ValueType>::min();
    }
};

#endif  // PYVRP_MEASURE_H
