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

    value_type const value;

public:
    Measure(value_type const value) : value(value){};

    // Explicit conversions to other measures.
    explicit operator Measure<MeasureType::DISTANCE>() const { return {value}; }
    explicit operator Measure<MeasureType::DURATION>() const { return {value}; }
    explicit operator Measure<MeasureType::COST>() const { return {value}; }

    // Explicit conversion to the underlying value.
    explicit operator value_type() const { return value; }

    // Arithmetic operators.
    Measure operator+(Measure const &other) { return {value + other.value}; }
    Measure operator-(Measure const &other) { return {value - other.value}; }
    Measure operator*(Measure const &other) { return {value * other.value}; }
    Measure operator/(Measure const &other) { return {value / other.value}; }

    // Comparison operators.
    auto operator==(Measure const &other) { return value == other.value; }
    auto operator<=>(Measure const &other) { return value <=> other.value; }
};

using distance_type = Measure<MeasureType::DISTANCE>;
using duration_type = Measure<MeasureType::DURATION>;
using cost_type = Measure<MeasureType::COST>;
