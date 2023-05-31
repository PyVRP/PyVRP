#ifndef MEASURE_H
#define MEASURE_H

#include <compare>
#include <functional>
#include <iostream>
#include <type_traits>

using Value = int;

enum class MeasureType
{
    COORD,
    DIST,
    DURATION,
    COST,
    LOAD,
};

/**
 * The measure class is a thin wrapper around an underlying ``Value``. The
 * measure forms a strong type that is only explicitly convertible into other
 * arithmetic or measure types.
 */
template <MeasureType _> class Measure
{
    friend class Measure<MeasureType::COORD>;
    friend class Measure<MeasureType::COST>;
    friend class Measure<MeasureType::DIST>;
    friend class Measure<MeasureType::DURATION>;
    friend class Measure<MeasureType::LOAD>;

    Value value;

public:
    // Default construction initialises to 0.
    Measure() = default;

    // This constructor takes any arithmetic type (generally useful) and casts
    // its value to Value.
    template <typename T,
              std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    Measure(T const value) : value(static_cast<Value>(value))
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
    explicit operator Measure<MeasureType::COORD>() const { return {value}; };
    explicit operator Measure<MeasureType::COST>() const { return {value}; }
    explicit operator Measure<MeasureType::DIST>() const { return {value}; }
    explicit operator Measure<MeasureType::DURATION>() const { return {value}; }
    explicit operator Measure<MeasureType::LOAD>() const { return {value}; }

    // Comparison operators.
    auto operator==(Measure const &other) const { return value == other.value; }
    auto operator<=>(Measure const &other) const
    {
        // TODO if we implement inexact equality for doubles, we also need to
        //  update hashing to avoid two objects comparing equal with different
        //  hash values).
        return value <=> other.value;
    }
};

template <MeasureType Type>
Measure<Type> operator+(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return static_cast<Value>(lhs) + static_cast<Value>(rhs);
}

template <MeasureType Type> Measure<Type> operator+(Measure<Type> const lhs)
{
    return +static_cast<Value>(lhs);
}

template <MeasureType Type>
Measure<Type> operator-(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return static_cast<Value>(lhs) - static_cast<Value>(rhs);
}

template <MeasureType Type> Measure<Type> operator-(Measure<Type> const lhs)
{
    return -static_cast<Value>(lhs);
}

template <MeasureType Type>
Measure<Type> operator*(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return static_cast<Value>(lhs) * static_cast<Value>(rhs);
}

template <MeasureType Type>
Measure<Type> operator/(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return static_cast<Value>(lhs) / static_cast<Value>(rhs);
}

// For printing.
template <MeasureType Type>
std::ostream &operator<<(std::ostream &out, Measure<Type> const measure)
{
    return out << static_cast<Value>(measure);
}

namespace std
{
template <MeasureType Type> struct hash<Measure<Type>>
{
    size_t operator()(Measure<Type> const measure) const
    {
        return std::hash<Value>()(static_cast<Value>(measure));
    }
};
}  // namespace std

// Useful type aliases.
using Coordinate = Measure<MeasureType::COORD>;
using Cost = Measure<MeasureType::COST>;
using Distance = Measure<MeasureType::DIST>;
using Duration = Measure<MeasureType::DURATION>;
using Load = Measure<MeasureType::LOAD>;

#endif  // MEASURE_H
