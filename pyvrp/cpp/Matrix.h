#ifndef PYVRP_MATRIX_H
#define PYVRP_MATRIX_H

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace pyvrp
{
template <typename T> class Matrix
{
    size_t cols_;          // The number of columns of the matrix
    size_t rows_;          // The number of rows of the matrix
    std::vector<T> data_;  // Data vector

public:
    /**
     * Creates a square matrix of size dimension * dimension.
     *
     * @param dimension Size of one side.
     */
    explicit Matrix(size_t dimension);

    /**
     * Creates a matrix of size nRows * nCols.
     *
     * @param nRows Number of rows.
     * @param nCols Number of columns.
     */
    Matrix(size_t nRows, size_t nCols);

    /**
     * Creates a matrix from the given data. The data is copied into the
     * created matrix.
     *
     * @param Data elements.
     */
    explicit Matrix(std::vector<std::vector<T>> const &data);

    [[nodiscard]] decltype(auto) operator()(size_t row, size_t col);

    [[nodiscard]] decltype(auto) operator()(size_t row, size_t col) const;

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
Matrix<T>::Matrix(size_t dimension)
    : cols_(dimension), rows_(dimension), data_(dimension * dimension)
{
}

template <typename T>
Matrix<T>::Matrix(size_t nRows, size_t nCols)
    : cols_(nCols), rows_(nRows), data_(nRows * nCols)
{
}

template <typename T>
Matrix<T>::Matrix(std::vector<std::vector<T>> const &data)
    : Matrix(data.size(), data.empty() ? 0 : data[0].size())
{
    for (size_t i = 0; i != rows_; ++i)
    {
        auto const size = data[i].size();

        if (size != cols_)
        {
            std::ostringstream msg;
            msg << "Expected " << cols_ << "elements, got " << size << ".";
            throw std::invalid_argument(msg.str());
        }

        for (size_t j = 0; j != cols_; ++j)
            data_[cols_ * i + j] = data[i][j];
    }
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

template <typename T> size_t Matrix<T>::numCols() const { return cols_; }

template <typename T> size_t Matrix<T>::numRows() const { return rows_; }

template <typename T> T Matrix<T>::max() const
{
    return *std::max_element(data_.begin(), data_.end());
}

template <typename T> size_t Matrix<T>::size() const { return data_.size(); }
}  // namespace pyvrp

#endif  // PYVRP_MATRIX_H
