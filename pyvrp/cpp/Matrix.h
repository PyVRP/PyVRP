#ifndef PYVRP_MATRIX_H
#define PYVRP_MATRIX_H

#include <algorithm>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace pyvrp
{
template <typename T> class Matrix
{
    size_t cols_ = 0;           // The number of columns of the matrix
    size_t rows_ = 0;           // The number of rows of the matrix
    std::vector<T> data_ = {};  // Data vector

public:
    Matrix() = default;  // default is an empty matrix

    /**
     * Creates a matrix of size nRows * nCols.
     *
     * @param nRows Number of rows.
     * @param nCols Number of columns.
     */
    explicit Matrix(size_t nRows, size_t nCols);

    explicit Matrix(std::vector<T> data, size_t nRows, size_t nCols);

    [[nodiscard]] decltype(auto) operator()(size_t row, size_t col);
    [[nodiscard]] decltype(auto) operator()(size_t row, size_t col) const;

    [[nodiscard]] T *data();
    [[nodiscard]] T const *data() const;

    [[nodiscard]] size_t numCols() const;

    [[nodiscard]] size_t numRows() const;

    /**
     * @return Maximum element in the matrix.
     */
    [[nodiscard]] T max() const;

    /**
     * @return Matrix size.
     */
    [[nodiscard]] size_t size() const;
};

template <typename T>
Matrix<T>::Matrix(size_t nRows, size_t nCols)
    : cols_(nCols), rows_(nRows), data_(nRows * nCols)
{
}

template <typename T>
Matrix<T>::Matrix(std::vector<T> data, size_t nRows, size_t nCols)
    : cols_(nCols), rows_(nRows), data_(std::move(data))
{
    assert(cols_ * rows_ == data_.size());
}

template <typename T>
decltype(auto) Matrix<T>::operator()(size_t row, size_t col)
{
    return data_[cols_ * row + col];
}

template <typename T>
decltype(auto) Matrix<T>::operator()(size_t row, size_t col) const
{
    return data_[cols_ * row + col];
}

template <typename T> T *Matrix<T>::data() { return data_.data(); }
template <typename T> T const *Matrix<T>::data() const { return data_.data(); }

template <typename T> size_t Matrix<T>::numCols() const { return cols_; }

template <typename T> size_t Matrix<T>::numRows() const { return rows_; }

template <typename T> T Matrix<T>::max() const
{
    return *std::max_element(data_.begin(), data_.end());
}

template <typename T> size_t Matrix<T>::size() const { return data_.size(); }
}  // namespace pyvrp

#endif  // PYVRP_MATRIX_H
