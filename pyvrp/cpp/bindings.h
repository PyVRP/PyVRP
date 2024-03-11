#include "Matrix.h"
#include "Measure.h"

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <type_traits>

namespace pybind11::detail
{
// This is not a fully general type caster for Matrix. Instead, it assumes
// we're casting elements that are, or have the same size as, pyvrp::Value,
// which is the case for e.g. the Measure types.
template <typename T> struct type_caster<pyvrp::Matrix<T>>
{
    static_assert(sizeof(T) == sizeof(pyvrp::Value)
                  && std::is_constructible_v<T, pyvrp::Value>);

    PYBIND11_TYPE_CASTER(pyvrp::Matrix<T>, _("numpy.ndarray[int]"));

    bool load(pybind11::handle src, bool convert)  // Python -> C++
    {
        if (!convert && !pybind11::array_t<pyvrp::Value>::check_(src))
            return false;

        auto const style
            = pybind11::array::c_style | pybind11::array::forcecast;
        auto const buf = pybind11::array_t<pyvrp::Value, style>::ensure(src);

        if (!buf || buf.ndim() != 2)
            throw pybind11::value_error("Expected 2D np.ndarray argument!");

        if (buf.size() == 0)  // then the default constructed object is already
            return true;      // OK, and we have nothing to do.

        std::vector<T> data = {buf.data(), buf.data() + buf.size()};
        value = pyvrp::Matrix<T>(data, buf.shape(0), buf.shape(1));

        return true;
    }

    static pybind11::handle
    cast(pyvrp::Matrix<T> const &src,  // C++ -> Python
         [[maybe_unused]] pybind11::return_value_policy policy,
         pybind11::handle parent)
    {
        auto constexpr elemSize = sizeof(pyvrp::Value);

        pybind11::array_t<pyvrp::Value> array
            = {{src.numRows(), src.numCols()},                      // shape
               {elemSize * src.numCols(), elemSize},                // strides
               reinterpret_cast<pyvrp::Value const *>(src.data()),  // data
               parent};                                             // base

        // This is not pretty, but it makes the matrix non-writeable on the
        // Python side. That's needed because src is const, and we should
        // preserve that to avoid issues.
        pybind11::detail::array_proxy(array.ptr())->flags
            &= ~pybind11::detail::npy_api::NPY_ARRAY_WRITEABLE_;

        return array.release();
    }
};

// On the C++ side we have strong types for different measure values (for
// example distance, duration, etc.), but on the Python side those things
// are just ints. This type caster converts between the two.
template <pyvrp::MeasureType T> struct type_caster<pyvrp::Measure<T>>
{
    PYBIND11_TYPE_CASTER(pyvrp::Measure<T>, _("int"));

    bool load(pybind11::handle src, bool convert)  // Python -> C++
    {
        if (!convert && !PyLong_Check(src.ptr()))  // only int when conversion
            return false;                          // is not allowed.

        PyObject *tmp = PyNumber_Long(src.ptr());  // any argument for which
        if (!tmp)                                  // Python's int() succeeds.
            return false;

        auto const raw = PyLong_AsLong(tmp);
        Py_DECREF(tmp);

        value = pyvrp::Measure<T>(raw);
        return !PyErr_Occurred();
    }

    static pybind11::handle
    cast(pyvrp::Measure<T> const &src,  // C++ -> Python
         [[maybe_unused]] pybind11::return_value_policy policy,
         [[maybe_unused]] pybind11::handle parent)
    {
        return PyLong_FromLong(src.get());
    }
};
}  // namespace pybind11::detail
