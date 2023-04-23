#include <compare>

#ifdef INT_PRECISION
using ValueType = int;
#else
using ValueType = double;
#endif

enum MeasureType
{
    DISTANCE,
    DURATION,
    COST,
};

template <MeasureType _> class Measure
{
    friend class Measure<DISTANCE>;
    friend class Measure<DURATION>;
    friend class Measure<COST>;

    ValueType const value;

public:
    Measure(ValueType const value) : value(value){};

    // Explicit conversions to other measures.
    explicit operator Measure<DISTANCE>() const { return {value}; }
    explicit operator Measure<DURATION>() const { return {value}; }
    explicit operator Measure<COST>() const { return {value}; }

    // Explicit conversion to the underlying value.
    explicit operator ValueType() const { return value; }

    // Arithmetic operators.
    Measure operator+(Measure const &other) { return {value + other.value}; }
    Measure operator-(Measure const &other) { return {value - other.value}; }
    Measure operator*(Measure const &other) { return {value * other.value}; }
    Measure operator/(Measure const &other) { return {value / other.value}; }

    // Comparison operators.
    auto operator==(Measure const &other) { return value == other.value; }
    auto operator<=>(Measure const &other) { return value <=> other.value; }
};

using distance_type = Measure<DISTANCE>;
using duration_type = Measure<DURATION>;
using cost_type = Measure<COST>;
