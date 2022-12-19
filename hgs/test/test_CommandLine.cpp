#include <gtest/gtest.h>

#include "CommandLine.h"

#include <stdexcept>
#include <string>
#include <vector>

TEST(TestCommandLine, ctorThrows)
{
    for (auto argc = 0; argc != 3; ++argc) // less than three args should throw
        ASSERT_THROW(CommandLine(argc, nullptr), std::invalid_argument);

    for (auto argc = 4; argc != 10; argc += 2)  // even arg count should throw
        ASSERT_THROW(CommandLine(argc, nullptr), std::invalid_argument);
}

// TODO test instPath, solPath

// TODO test config parsing
