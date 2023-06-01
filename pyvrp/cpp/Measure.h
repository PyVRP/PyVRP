#ifndef MEASURE_H
#define MEASURE_H

#include <compare>
#include <functional>
#include <iostream>
#include <limits>
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

// Forward declaration so we can define the relevant type aliases early.
template <MeasureType _> class Measure;

// Type aliases. These are used throughout the program.
using Coordinate = Measure<MeasureType::COORD>;
using Cost = Measure<MeasureType::COST>;
using Distance = Measure<MeasureType::DIST>;
using Duration = Measure<MeasureType::DURATION>;
using Load = Measure<MeasureType::LOAD>;

//
//                 EVERYTHING BELOW THIS IS IMPLEMENTATION
//

/**
 * The measure class is a thin wrapper around an underlying ``Value``. The
 * measure forms a strong type that is only explicitly convertible into other
 * arithmetic or measure types.
 */
template <MeasureType _> class Measure
{
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

    // Explicit conversions to other measures.
    template <MeasureType Other> explicit operator Measure<Other>() const
    {
        return value;
    }

    // Retreives the underlying value.
    Value get() const;

    // In-place unary operators.
    Measure &operator+=(Measure const &rhs);
    Measure &operator-=(Measure const &rhs);
    Measure &operator*=(Measure const &rhs);
    Measure &operator/=(Measure const &rhs);

    // Comparison operators.
    auto operator==(Measure const &other) const;
    auto operator<=>(Measure const &other) const;
};

// Retreives the underlying value.
template <MeasureType Type> Value Measure<Type>::get() const { return value; }

// In-place unary operators.
template <MeasureType Type>
Measure<Type> &Measure<Type>::operator+=(Measure<Type> const &rhs)
{
    this->value += rhs.value;
    return *this;
}

template <MeasureType Type>
Measure<Type> &Measure<Type>::operator-=(Measure<Type> const &rhs)
{
    this->value -= rhs.value;
    return *this;
}

template <MeasureType Type>
Measure<Type> &Measure<Type>::operator*=(Measure<Type> const &rhs)
{
    this->value *= rhs.value;
    return *this;
}

template <MeasureType Type>
Measure<Type> &Measure<Type>::operator/=(Measure<Type> const &rhs)
{
    this->value /= rhs.value;
    return *this;
}

// Comparison operators.

template <MeasureType Type>
auto Measure<Type>::operator==(Measure<Type> const &other) const
{
    // TODO if we implement inexact equality for doubles, we also need to
    //  update hashing to avoid two objects comparing equal but having
    //  different hash values.
    return value == other.value;
}

template <MeasureType Type>
auto Measure<Type>::operator<=>(Measure<Type> const &other) const
{
    return value <=> other.value;
}

// Free-standing binary operators.
template <MeasureType Type>
Measure<Type> operator+(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return lhs.get() + rhs.get();
}

template <MeasureType Type> Measure<Type> operator+(Measure<Type> const lhs)
{
    return +lhs.get();
}

template <MeasureType Type>
Measure<Type> operator-(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return lhs.get() - rhs.get();
}

template <MeasureType Type> Measure<Type> operator-(Measure<Type> const lhs)
{
    return -lhs.get();
}

template <MeasureType Type>
Measure<Type> operator*(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return lhs.get() * rhs.get();
}

template <MeasureType Type>
Measure<Type> operator/(Measure<Type> const lhs, Measure<Type> const rhs)
{
    return lhs.get() / rhs.get();
}

// For printing.
template <MeasureType Type>
std::ostream &operator<<(std::ostream &out, Measure<Type> const measure)
{
    return out << measure.get();
}

// Specialisations for hashing and numerical limits.
namespace std
{
template <MeasureType Type> struct hash<Measure<Type>>
{
    size_t operator()(Measure<Type> const measure) const
    {
        return std::hash<Value>()(measure.get());
    }
};

template <MeasureType Type> class numeric_limits<Measure<Type>>
{
public:  // TODO should return type be Measure<Type>?
    static Value min() { return std::numeric_limits<Value>::min(); }
    static Value max() { return std::numeric_limits<Value>::max(); }
};

}  // namespace std

#endif  // MEASURE_H
