cmake_minimum_required(VERSION 3.22.0)
project(HUAWEI)

# Google Test
# include(FetchContent)
# FetchContent_Declare(
#   googletest
#   # Specify the commit you depend on and update it regularly.
#   URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
# )
# # For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

# debug
set(CMAKE_C_COMPILER gcc )
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17")
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
add_subdirectory(LoadGenerator)


# add_subdirectory(test)
# add_subdirectory(gasa)
