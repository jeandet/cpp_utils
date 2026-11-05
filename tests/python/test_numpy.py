import gc
import unittest

import numpy as np

import _cpp_utils_python_test as m


class TestNumpy(unittest.TestCase):
    def test_view_values_and_shape(self):
        a = m.make_iota(4, 3)
        self.assertEqual(a.shape, (4, 3))
        self.assertEqual(a.dtype, np.float64)
        np.testing.assert_array_equal(a.ravel(), np.arange(12.0))

    def test_array_outlives_producer(self):
        a = m.make_iota(1000, 3)
        gc.collect()
        self.assertEqual(a[-1, -1], 2999.0)

    def test_empty(self):
        self.assertEqual(m.make_iota(0, 3).shape, (0, 3))

    def test_fast_allocate_shape(self):
        self.assertEqual(m.make_uninit(5).shape, (5,))

    def test_view_is_writable_and_contiguous(self):
        a = m.make_iota(2, 3)
        self.assertTrue(a.flags.writeable)
        self.assertTrue(a.flags.c_contiguous)
        a[0, 0] = 42.0
        self.assertEqual(a[0, 0], 42.0)


if __name__ == "__main__":
    unittest.main()
