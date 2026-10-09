#ifndef PYVRP_MEASURE_H
#define PYVRP_MEASURE_H

#include <compare>
#include <format>
#include <limits>
#include <ostream>
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
template <MeasureType Type> class Measure;

// Type aliases. These are used throughout the program.
using Coordinate = Measure<MeasureType::COORD>;
using Cost = Measure<MeasureType::COST>;
using Distance = Measure<MeasureType::DIST>;
using Duration = Measure<MeasureType::DURATION>;
using Load = Measure<MeasureType::LOAD>;

//
//                 EVERYTHING BELOW IS AN IMPLEMENTATION DETAIL
//

/**
 * The measure class is a thin wrapper around an underlying double value. The
 * measure forms a strong type that is only explicitly castable to other
 * arithmetic or measure types.
 *
 * The measure is equipped with a ``MeasureType`` that specifies what it is
 * intended to model. Comparisons use the underlying double values exactly.
 * Any rounding tolerance must be applied explicitly at the comparison site.
 */
template <MeasureType _> class Measure
{
    double value_ = 0;

public:
    // Default construction initialises to 0.
    Measure() = default;

    // Construct from any arithmetic type.
    template <NumberType T>
    Measure(T const value) : value_(static_cast<double>(value))
    {
    }

    // Explicit conversions of the underlying value to other arithmetic types.
    template <NumberType T> explicit operator T() const
    {
        return static_cast<T>(value_);
    }

    // Explicit conversions to other measures preserve the underlying value.
    template <MeasureType Other> explicit operator Measure<Other>() const
    {
        return value_;
    }

    // Retrieves the underlying value.
    [[nodiscard]] double get() const;

    // In-place unary operators.
    Measure &operator+=(Measure const rhs);
    Measure &operator-=(Measure const rhs);
    Measure &operator*=(Measure const rhs);
    Measure &operator/=(Measure const rhs);

    // Comparison operators.
    [[nodiscard]] std::partial_ordering operator<=>(Measure const &other) const
        = default;
    [[nodiscard]] bool operator==(Measure const &other) const = default;
};

// Retrieves the underlying value.
template <MeasureType Type> double Measure<Type>::get() const { return value_; }

// In-place unary operators.
template <MeasureType Type>
Measure<Type> &Measure<Type>::operator+=(Measure<Type> const rhs)
{
    this->value_ += rhs.value_;
    return *this;
}

template <MeasureType Type>
Measure<Type> &Measure<Type>::operator-=(Measure<Type> const rhs)
{
    this->value_ -= rhs.value_;
    return *this;
}

template <MeasureType Type>
Measure<Type> &Measure<Type>::operator*=(Measure<Type> const rhs)
{
    this->value_ *= rhs.value_;
    return *this;
}

template <MeasureType Type>
Measure<Type> &Measure<Type>::operator/=(Measure<Type> const rhs)
{
    this->value_ /= rhs.value_;
    return *this;
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
}  // namespace pyvrp

// For printing.
template <pyvrp::MeasureType Type>
std::ostream &operator<<(std::ostream &out, pyvrp::Measure<Type> const measure)
{
    return out << measure.get();
}

// Specialisations for numerical limits and formatting.

template <pyvrp::MeasureType Type>
class std::numeric_limits<pyvrp::Measure<Type>>
{
public:
    static pyvrp::Measure<Type> max()
    {
        return std::numeric_limits<double>::max();
    }
};

template <pyvrp::MeasureType Type>
struct std::formatter<pyvrp::Measure<Type>> : std::formatter<double>
{
    auto format(pyvrp::Measure<Type> const measure, auto &ctx) const
    {
        return std::formatter<double>::format(measure.get(), ctx);
    }
};

#endif  // PYVRP_MEASURE_H
