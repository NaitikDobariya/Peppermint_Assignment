cmake_minimum_required(VERSION 3.10)
project(PeppermintLanePlanner)

# Ground Rule: C++17 or newer is non-negotiable
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include header files
include_directories(include)

# Define source files
set(SOURCES
    src/main.cpp
    src/graph.cpp
    src/planner.cpp
    src/json_exporter.cpp
)

# Build the standalone executable
add_executable(run_simulation ${SOURCES})