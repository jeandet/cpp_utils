/*------------------------------------------------------------------------------
-- The MIT License (MIT)
--
-- Copyright © 2024, Laboratory of Plasma Physics- CNRS
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the “Software”), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
-- of the Software, and to permit persons to whom the Software is furnished to do
-- so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
-- INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
-- PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
-- HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
-- OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
-- SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-------------------------------------------------------------------------------*/
/*-- Author : Alexis Jeandet
-- Mail : alexis.jeandet@member.fsf.org
----------------------------------------------------------------------------*/
#pragma once
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <chrono>
#include <cstdint>

#include <datetime.h>

// Format-agnostic datetime helpers: CPython's datetime.datetime <-> C++ time points, and an int64
// nanosecond buffer viewed as numpy datetime64[ns]. Naive datetimes are UTC (pybind11's own chrono
// caster reads them as local time); aware ones are shifted to UTC through utcoffset().
namespace cpp_utils::python
{
// datetime.h stores the C API in a per-translation-unit static, so the import has to be attempted
// from the code that uses it rather than once at module init.
inline bool ensure_datetime_api()
{
    if (PyDateTimeAPI == nullptr)
        PyDateTime_IMPORT;
    return PyDateTimeAPI != nullptr;
}

// A datetime.datetime argument, converted with to_time_point.
struct py_datetime
{
    const PyObject* obj;
};

template <typename T>
concept time_point_type = requires(T a) { a.time_since_epoch(); };

[[nodiscard]] inline PyObject* to_datetime(const time_point_type auto& timepoint)
{
    using namespace std::chrono;
    ensure_datetime_api();
    const auto dp = floor<days>(timepoint);
    year_month_day ymd { dp };
    hh_mm_ss time { floor<microseconds>(timepoint - dp) };
    return PyDateTime_FromDateAndTime(static_cast<int>(ymd.year()),
        static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day()),
        static_cast<unsigned>(time.hours().count()), static_cast<unsigned>(time.minutes().count()),
        static_cast<unsigned>(time.seconds().count()),
        static_cast<unsigned>(time.subseconds().count())); // microseconds
}

[[nodiscard]] inline PyObject* to_datetime(std::int64_t ns_since_1970)
{
    return to_datetime(std::chrono::sys_time<std::chrono::nanoseconds> {
        std::chrono::nanoseconds { ns_since_1970 } });
}

[[nodiscard]] inline std::chrono::microseconds utc_offset(const PyDateTime_DateTime* dt)
{
    auto offset = pybind11::reinterpret_steal<pybind11::object>(PyObject_CallMethod(
        reinterpret_cast<PyObject*>(const_cast<PyDateTime_DateTime*>(dt)), "utcoffset", nullptr));
    if (!offset)
        throw pybind11::error_already_set();
    if (offset.is_none())
        return {};
    return std::chrono::days { PyDateTime_DELTA_GET_DAYS(offset.ptr()) }
        + std::chrono::seconds { PyDateTime_DELTA_GET_SECONDS(offset.ptr()) }
        + std::chrono::microseconds { PyDateTime_DELTA_GET_MICROSECONDS(offset.ptr()) };
}

[[nodiscard]] inline std::chrono::sys_time<std::chrono::nanoseconds> to_time_point(
    const PyObject* dt)
{
    using namespace std::chrono;
    ensure_datetime_api();
    const auto* date_time = reinterpret_cast<const PyDateTime_DateTime*>(dt);
    const sys_days date = year_month_day { year { PyDateTime_GET_YEAR(date_time) },
        month { static_cast<unsigned>(PyDateTime_GET_MONTH(date_time)) },
        day { static_cast<unsigned>(PyDateTime_GET_DAY(date_time)) } };
    const auto wall_clock = date + hours { PyDateTime_DATE_GET_HOUR(date_time) }
        + minutes { PyDateTime_DATE_GET_MINUTE(date_time) }
        + seconds { PyDateTime_DATE_GET_SECOND(date_time) }
        + microseconds { PyDateTime_DATE_GET_MICROSECOND(date_time) };
    const auto utc = date_time->hastzinfo ? wall_clock - utc_offset(date_time) : wall_clock;
    return sys_time<nanoseconds> { duration_cast<nanoseconds>(utc.time_since_epoch()) };
}

// Same buffer, datetime64[ns] dtype: no copy, the result keeps the input array alive as its base.
[[nodiscard]] inline pybind11::array as_datetime64_ns(const pybind11::array_t<std::int64_t>& ns)
{
    return pybind11::reinterpret_borrow<pybind11::array>(
        pybind11::object(ns.attr("view")("datetime64[ns]")));
}
}

namespace pybind11::detail
{
template <>
struct type_caster<cpp_utils::python::py_datetime>
{
    PYBIND11_TYPE_CASTER(cpp_utils::python::py_datetime, const_name("datetime.datetime"));

    bool load(handle src, bool)
    {
        if (!cpp_utils::python::ensure_datetime_api() || !PyDateTime_Check(src.ptr()))
            return false;
        value.obj = src.ptr();
        return true;
    }

    static handle cast(const cpp_utils::python::py_datetime& dt, return_value_policy, handle)
    {
        return pybind11::handle(const_cast<PyObject*>(dt.obj)).inc_ref();
    }
};
}
