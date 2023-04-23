#include <compare>

#ifdef INT_PRECISION
using value_type = int;
#else
using value_type = double;
#endif

enum class MeasureType
{
    DISTANCE,
    DURATION,
    COST,
};

template <MeasureType _> class Measure
{
    friend class Measure<MeasureType::DISTANCE>;
    friend class Measure<MeasureType::DURATION>;
    friend class Measure<MeasureType::COST>;

    value_type value;

public:
    // Default construction initialises to 0. The other constructors take both
    // precisions (generally useful), but do case that value to the underlying
    // value type.
    Measure() = default;
    Measure(int const value) : value(static_cast<value_type>(value)){};
    Measure(double const value) : value(static_cast<value_type>(value)){};

    // Explicit conversion of the underlying value to int and double.
    explicit operator int() const { return static_cast<int>(value); }
    explicit operator double() const { return static_cast<double>(value); }

    // Explicit conversions to other measures.
    explicit operator Measure<MeasureType::DISTANCE>() const { return {value}; }
    explicit operator Measure<MeasureType::DURATION>() const { return {value}; }
    explicit operator Measure<MeasureType::COST>() const { return {value}; }

    // Arithmetic operators.
    Measure operator+(Measure const &other) const
    {
        return {value + other.value};
    }

    Measure operator-(Measure const &other) const
    {
        return {value - other.value};
    }

    Measure operator*(Measure const &other) const
    {
        return {value * other.value};
    }

    Measure operator/(Measure const &other) const
    {
        return {value / other.value};
    }

    // Comparison operators.
    auto operator==(Measure const &other) const { return value == other.value; }
    auto operator<=>(Measure const &other) const
    {
        return value <=> other.value;
    }
};

using distance_type = Measure<MeasureType::DISTANCE>;
using duration_type = Measure<MeasureType::DURATION>;
using cost_type = Measure<MeasureType::COST>;
