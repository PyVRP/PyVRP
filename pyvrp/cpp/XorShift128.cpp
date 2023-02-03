#include "XorShift128.h"

#include <pybind11/pybind11.h>

namespace py = pybind11;

XorShift128::XorShift128(int seed)
{
    state_[0] = seed;
    state_[1] = 123456789;
    state_[2] = 362436069;
    state_[3] = 521288629;
}

XorShift128::result_type XorShift128::operator()()
{
    // Algorithm "xor128" from p. 5 of Marsaglia, "Xorshift RNGs"
    unsigned t = state_[3];

    // Perform a contrived 32-bit shift.
    unsigned s = state_[0];
    state_[3] = state_[2];
    state_[2] = state_[1];
    state_[1] = s;

    t ^= t << 11;
    t ^= t >> 8;

    // Return the new random number
    return state_[0] = t ^ s ^ (s >> 19);
}

PYBIND11_MODULE(XorShift128, m)
{
    py::class_<XorShift128>(m, "XorShift128")
        .def(py::init<int>(), py::arg("seed"))
        .def("min", &XorShift128::min)
        .def("max", &XorShift128::max)
        .def("__call__", &XorShift128::operator())
        .def("rand", &XorShift128::rand<double>)
        .def("randint", &XorShift128::randint<int>, py::arg("high"));
}
