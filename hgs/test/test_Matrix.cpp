#include <gtest/gtest.h>

#include "Matrix.h"

TEST(TestMatrix, Constructor)
{
    Matrix<int> square(10);
    ASSERT_EQ(square.size(), 10 * 10);
    ASSERT_EQ(square.max(), 0);  // matrix initialises all zero

    Matrix<int> rectangle(10, 20);
    ASSERT_EQ(rectangle.size(), 10 * 20);
    ASSERT_EQ(square.max(), 0);  // matrix initialises all zero
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
