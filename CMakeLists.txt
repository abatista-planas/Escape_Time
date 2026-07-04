cmake_minimum_required(VERSION 3.16)

project(Escape_Time LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# This program is compute-bound; default to an optimized build.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(Escape_Time main.cpp)

# --- Parallel backends: each is optional and enabled only when found ---

# OpenMP -> per-rank threading (mode 2)
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  target_link_libraries(Escape_Time PRIVATE OpenMP::OpenMP_CXX)
else()
  message(STATUS "OpenMP not found: mode 2 will run sequentially.")
endif()

# MPI -> row-block decomposition across processes
find_package(MPI)
if(MPI_CXX_FOUND)
  target_link_libraries(Escape_Time PRIVATE MPI::MPI_CXX)
  target_compile_definitions(Escape_Time PRIVATE USE_MPI)
else()
  message(STATUS "MPI not found: building a single-process executable.")
endif()

# TBB -> parallel STL backend (mode 1)
find_package(TBB QUIET)
if(TBB_FOUND AND TARGET TBB::tbb)
  target_link_libraries(Escape_Time PRIVATE TBB::tbb)
  target_compile_definitions(Escape_Time PRIVATE USE_TBB)
else()
  message(STATUS "TBB (TBB::tbb) not found: mode 1 will run sequentially.")
endif()

find_package(Threads REQUIRED)
target_link_libraries(Escape_Time PRIVATE Threads::Threads)
