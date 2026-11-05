#include <pybind11/pybind11.h>

#include <array>
#include <chrono>
#include <memory>
#include <numeric>
#include <vector>

#include "cpp_utils_python/datetime.hpp"
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

py::object ns_to_datetime(std::int64_t ns)
{
    return py::reinterpret_steal<py::object>(cpp_utils::python::to_datetime(ns));
}

std::int64_t datetime_to_ns(const py::object& dt)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        cpp_utils::python::to_time_point(dt.ptr()).time_since_epoch())
        .count();
}

py::array as_dt64(py::array_t<std::int64_t> ns)
{
    return cpp_utils::python::as_datetime64_ns(ns);
}
}

PYBIND11_MODULE(_cpp_utils_python_test, m)
{
    m.def("make_iota", &make_iota);
    m.def("make_uninit", &make_uninit);
    m.def("ns_to_datetime", &ns_to_datetime);
    m.def("datetime_to_ns", &datetime_to_ns);
    m.def("as_dt64", &as_dt64);
}
