from numpy.testing import assert_equal, assert_raises

from pyvrp._pyvrp import Matrix


def test_dimension_constructor():
    square = Matrix(10)
    assert_equal(square.num_rows, 10)
    assert_equal(square.num_cols, 10)
    assert_equal(square.size(), 10 * 10)
    assert_equal(square.max(), 0)  # matrix initialises all zero

    rectangle = Matrix(10, 20)
    assert_equal(rectangle.num_rows, 10)
    assert_equal(rectangle.num_cols, 20)
    assert_equal(rectangle.size(), 10 * 20)
    assert_equal(rectangle.max(), 0)  # matrix initialises all zero


def test_data_constructor():
    empty = Matrix([[], []])
    assert_equal(empty.size(), 0)

    non_empty = Matrix([[1, 2], [1, 3]])
    assert_equal(non_empty.size(), 4)  # matrix has four elements
    assert_equal(non_empty.max(), 3)


def test_data_constructor_throws():
    with assert_raises(ValueError):
        Matrix([[1, 2], []])  # second row shorter than first

    with assert_raises(ValueError):
        Matrix([[1, 2], [1, 2, 3]])  # second row longer than first


def test_element_access():
    mat = Matrix(10)

    for i in range(10):
        for j in range(10):
            mat[i, j] = i + j

    assert_equal(mat.max(), 9 + 9)  # maximum value

    assert_equal(mat[1, 1], 1 + 1)  # several elements
    assert_equal(mat[2, 1], 2 + 1)
    assert_equal(mat[1, 2], 1 + 2)
