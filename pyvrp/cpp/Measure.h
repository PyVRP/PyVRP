#ifndef MEASURE_H
#define MEASURE_H

#include <compare>
#include <iostream>
#include <type_traits>

using value_type = int;

enum class MeasureType
{
    DISTANCE,
    DURATION,
    COST,
    CAPACITY,
};

/**
 * The measure class is a thin wrapper around an underlying value of type
 * ``value_type``. The measure forms a strong type that is only explicitly
 * convertible into other arithmetic or measure types.
 */
template <MeasureType _> class Measure
{
    friend class Measure<MeasureType::DISTANCE>;
    friend class Measure<MeasureType::DURATION>;
    friend class Measure<MeasureType::COST>;
    friend class Measure<MeasureType::CAPACITY>;

    value_type value;

public:
    // Default construction initialises to 0.
    Measure() = default;

    // This constructor takes any arithmetic type (generally useful) and casts
    // its value to value_type.
    template <typename T,
              std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    Measure(T const value) : value(static_cast<value_type>(value))
    {
    }

    // Explicit conversions of the underlying value to other arithmetic types.
    template <typename T,
              std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    explicit operator T() const
    {
        return static_cast<T>(value);
    }

    Measure &operator+=(Measure const &rhs)
    {
        this->value += rhs.value;
        return *this;
    }

    Measure &operator-=(Measure const &rhs)
    {
        this->value -= rhs.value;
        return *this;
    }

    Measure &operator*=(Measure const &rhs)
    {
        this->value *= rhs.value;
        return *this;
    }

    Measure &operator/=(Measure const &rhs)
    {
        this->value /= rhs.value;
        return *this;
    }

    // Explicit conversions to other measures.
    explicit operator Measure<MeasureType::DISTANCE>() const { return {value}; }
    explicit operator Measure<MeasureType::DURATION>() const { return {value}; }
    explicit operator Measure<MeasureType::COST>() const { return {value}; }
    explicit operator Measure<MeasureType::CAPACITY>() const { return {value}; }

    // Comparison operators.
    auto operator==(Measure const &other) const { return value == other.value; }
    auto operator<=>(Measure const &other) const
    {
        return value <=> other.value;
    }
};

template <MeasureType Type>
Measure<Type> operator+(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return static_cast<value_type>(lhs) + static_cast<value_type>(rhs);
}

template <MeasureType Type> Measure<Type> operator+(Measure<Type> const lhs)
{
    return +static_cast<value_type>(lhs);
}

template <MeasureType Type>
Measure<Type> operator-(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return static_cast<value_type>(lhs) - static_cast<value_type>(rhs);
}

template <MeasureType Type> Measure<Type> operator-(Measure<Type> const lhs)
{
    return -static_cast<value_type>(lhs);
}

template <MeasureType Type>
Measure<Type> operator*(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return static_cast<value_type>(lhs) * static_cast<value_type>(rhs);
}

template <MeasureType Type>
Measure<Type> operator/(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return static_cast<value_type>(lhs) / static_cast<value_type>(rhs);
}

// For printing.
template <MeasureType Type>
std::ostream &operator<<(std::ostream &out, Measure<Type> const measure)
{
    return out << static_cast<value_type>(measure);
}

// Useful type aliases.
using distance_type = Measure<MeasureType::DISTANCE>;
using duration_type = Measure<MeasureType::DURATION>;
using cost_type = Measure<MeasureType::COST>;
using capacity_type = Measure<MeasureType::CAPACITY>;

#endif  // MEASURE_H
