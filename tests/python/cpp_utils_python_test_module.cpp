#include <pybind11/pybind11.h>

#include <array>
#include <memory>
#include <numeric>
#include <vector>

#include "cpp_utils_python/numpy.hpp"

namespace py = pybind11;

namespace
{
py::array_t<double> make_iota(std::size_t records, std::size_t columns)
{
    auto values = std::make_shared<std::vector<double>>(records * columns);
    std::iota(std::begin(*values), std::end(*values), 0.);
    return cpp_utils::python::view_with_owner(values->data(),
        std::vector<ssize_t> { static_cast<ssize_t>(records), static_cast<ssize_t>(columns) },
        values);
}

py::array_t<double> make_uninit(std::size_t size)
{
    return cpp_utils::python::fast_allocate_array<double>(std::array<std::size_t, 1> { size });
}
}

PYBIND11_MODULE(_cpp_utils_python_test, m)
{
    m.def("make_iota", &make_iota);
    m.def("make_uninit", &make_uninit);
}
