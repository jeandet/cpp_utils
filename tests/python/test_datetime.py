import calendar
import datetime
import unittest
from datetime import timedelta, timezone

import numpy as np

import _cpp_utils_python_test as m


class TestDatetime(unittest.TestCase):
    def test_roundtrip_ns(self):
        self.assertEqual(m.ns_to_datetime(0), datetime.datetime(1970, 1, 1))
        self.assertEqual(
            m.ns_to_datetime(-378691200 * 10**9), datetime.datetime(1958, 1, 1)
        )

    def test_microsecond_floor(self):
        self.assertEqual(
            m.ns_to_datetime(1999), datetime.datetime(1970, 1, 1, 0, 0, 0, 1)
        )

    def test_aware_to_utc(self):
        dt = datetime.datetime(2001, 2, 1, 2, 0, tzinfo=timezone(timedelta(hours=2)))
        self.assertEqual(
            m.datetime_to_ns(dt), calendar.timegm((2001, 2, 1, 0, 0, 0)) * 10**9
        )

    def test_naive_is_utc(self):
        self.assertEqual(m.datetime_to_ns(datetime.datetime(1970, 1, 2)), 86400 * 10**9)

    def test_datetime64_view(self):
        a = m.as_dt64(np.array([0, 10**9], dtype=np.int64))
        self.assertEqual(a.dtype, np.dtype("datetime64[ns]"))
        self.assertEqual(a[1], np.datetime64("1970-01-01T00:00:01", "ns"))


if __name__ == "__main__":
    unittest.main()
