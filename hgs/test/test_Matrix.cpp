#include <gtest/gtest.h>

#include "Matrix.h"

TEST(TestMatrix, DimensionConstructors)
{
    Matrix<int> const square(10);
    ASSERT_EQ(square.size(), 10 * 10);
    ASSERT_EQ(square.max(), 0);  // matrix initialises all zero

    Matrix<int> const rectangle(10, 20);
    ASSERT_EQ(rectangle.size(), 10 * 20);
    ASSERT_EQ(square.max(), 0);  // matrix initialises all zero
}

TEST(TestMatrix, DataConstructor)
{
    std::vector<std::vector<int>> data = {{}, {}};
    Matrix<int> const empty(data);
    ASSERT_EQ(empty.size(), 0);

    Matrix<int> const nonEmpty({{1, 2}, {1, 2}});
    ASSERT_EQ(nonEmpty(0, 0), 1);
    ASSERT_EQ(nonEmpty(0, 1), 2);
}

TEST(TestMatrix, DataConstructorThrows)
{
    // Second row is shorter than first, so this should throw.
    ASSERT_THROW(Matrix<int>({{1, 2}, {1}}), std::invalid_argument);

    // Second row is longer than first, so this should also throw.
    ASSERT_THROW(Matrix<int>({{1, 2}, {1, 2, 3}}), std::invalid_argument);
}

TEST(TestMatrix, Elements)
{
    Matrix<int> m(10);

    for (auto i = 0; i != 10; ++i)
        for (auto j = 0; j != 10; ++j)
            m(i, j) = i + j;

    ASSERT_EQ(m.max(), 9 + 9);  // maximum value

    ASSERT_EQ(m(1, 1), 1 + 1);  // several elements
    ASSERT_EQ(m(2, 1), 2 + 1);
    ASSERT_EQ(m(1, 2), 1 + 2);
}
