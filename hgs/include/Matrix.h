#ifndef MATRIX_H
#define MATRIX_H

#include <vector>

// Implementation of a matrix in a C++ vector
// This class is used because a flat vector is faster than a vector of vectors
// which requires two lookup operations rather than one to index a matrix
// element
template <typename T> class Matrix
{
    size_t cols_;          // The number of columns of the matrix
    std::vector<T> data_;  // The vector where all the data is stored (this
                           // represents the matrix)

public:
    // Empty constructor: with zero columns and a vector of size zero
    Matrix() : cols_(0), data_(0) {}

    // Constructor: create a matrix of size dimension by dimension, using a C++
    // vector of size dimension * dimension.
    explicit Matrix(size_t dimension)
        : cols_(dimension), data_(dimension * dimension)
    {
    }

    Matrix(size_t nRows, size_t nCols) : cols_(nCols), data_(nRows * nCols) {}

    [[nodiscard]] decltype(auto) operator()(size_t row, size_t col)
    {
        return data_[cols_ * row + col];
    }

    [[nodiscard]] decltype(auto) operator()(size_t row, size_t col) const
    {
        return data_[cols_ * row + col];
    }

    [[nodiscard]] T max() const
    {
        return *std::max_element(data_.begin(), data_.end());
    }

    [[nodiscard]] size_t size() const { return data_.size(); }
};

#endif
