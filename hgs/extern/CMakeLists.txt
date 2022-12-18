cmake_minimum_required(VERSION 3.14)

include(FetchContent)

# Pybind (as a submodule)
add_subdirectory(pybind11)

# GoogleTest (via FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG 58d77fa8070e8cec2dc1ed015d66b454c8d78850  # v1.12.1
)

FetchContent_MakeAvailable(googletest)
