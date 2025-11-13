cmake_minimum_required(VERSION 3.10)

project(painlessMesh-simulator 
  VERSION 0.1.0
  DESCRIPTION "painlessMesh Device Simulator for large-scale mesh network testing"
  LANGUAGES CXX
)

# Set policy for FindBoost module (CMake 3.30+)
# NEW means use BoostConfig.cmake, OLD means use FindBoost.cmake
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ENABLE_TESTING "Enable unit testing" ON)
option(ENABLE_BENCHMARKS "Enable performance benchmarks" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(BUILD_EXAMPLES "Build example scenarios and firmware" ON)
option(BUILD_DOCS "Build documentation" OFF)

# Allow using a local painlessMesh clone instead of submodule
set(PAINLESSMESH_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/painlessMesh" CACHE PATH "Path to painlessMesh library")

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compiler flags
if(MSVC)
  # MSVC flags
  add_compile_options(/W4 /WX-)
  if(ENABLE_COVERAGE)
    message(WARNING "Code coverage is not supported on MSVC")
  endif()
else()
  # GCC/Clang flags
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
  )
  
  # Coverage flags
  if(ENABLE_COVERAGE)
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
  endif()
  
  # Debug flags
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
  endif()
  
  # Release flags
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG)
  endif()
endif()

# Find required packages
# Try to find Boost (Asio is header-only, but we need program_options for CLI)
find_package(Boost 1.66 REQUIRED COMPONENTS program_options)

# For compatibility, create Boost::system target if it doesn't exist
# Modern Boost (1.70+) with CMake config files may not provide it
if(NOT TARGET Boost::system)
  add_library(Boost::system INTERFACE IMPORTED)
  set_target_properties(Boost::system PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}"
  )
endif()

find_package(Threads REQUIRED)

# Find optional packages
find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
  message(STATUS "Found yaml-cpp: ${yaml-cpp_VERSION}")
else()
  message(STATUS "yaml-cpp not found, building from source...")
  include(FetchContent)
  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.7.0
  )
  FetchContent_MakeAvailable(yaml-cpp)
  set(yaml-cpp_FOUND TRUE)
endif()

find_package(Curses QUIET)
if(CURSES_FOUND)
  message(STATUS "Found ncurses, terminal UI will be enabled")
else()
  message(STATUS "ncurses not found, terminal UI will be disabled")
endif()

# Include directories
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${Boost_INCLUDE_DIRS}
)

# painlessMesh setup - check custom path or use submodule
if(NOT EXISTS "${PAINLESSMESH_PATH}/src")
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/painlessMesh/src")
    message(WARNING "painlessMesh not found at ${PAINLESSMESH_PATH}")
    message(STATUS "Attempting to initialize submodules...")
    execute_process(
      COMMAND git submodule update --init --recursive
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE GIT_SUBMOD_RESULT
    )
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
      message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}")
    endif()
    set(PAINLESSMESH_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/painlessMesh")
  else()
    set(PAINLESSMESH_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/painlessMesh")
  endif()
endif()

# Add painlessMesh library
# Since painlessMesh doesn't create a library target, we create an interface library
# that provides the include paths and dependencies needed for the simulator
if(EXISTS "${PAINLESSMESH_PATH}/src")
  add_library(painlessmesh INTERFACE)
  target_include_directories(painlessmesh INTERFACE
    ${PAINLESSMESH_PATH}/src
    ${PAINLESSMESH_PATH}/src/arduino
    ${PAINLESSMESH_PATH}/src/boost
  )
  target_link_libraries(painlessmesh INTERFACE
    Boost::system
    Threads::Threads
  )
  message(STATUS "painlessMesh library configured from: ${PAINLESSMESH_PATH}")
else()
  message(FATAL_ERROR "painlessMesh sources not found at ${PAINLESSMESH_PATH}")
endif()

# Simulator library (core components)
set(SIMULATOR_SOURCES
  # Core components
  ${PAINLESSMESH_PATH}/test/catch/fake_serial.cpp
  src/core/painlessmesh_support.cpp
  src/core/virtual_node.cpp
  src/core/node_manager.cpp
  src/config/config_loader.cpp
  # src/scenario/scenario_engine.cpp
  # src/network/network_simulator.cpp
  # src/firmware/firmware_base.cpp
  # src/metrics/metrics_collector.cpp
)

set(SIMULATOR_HEADERS
  # Header files
  include/simulator/virtual_node.hpp
  include/simulator/node_manager.hpp
  include/simulator/config_loader.hpp
  # include/simulator/scenario_engine.hpp
  # include/simulator/network_simulator.hpp
  # include/simulator/firmware_base.hpp
  # include/simulator/metrics_collector.hpp
)

# Create simulator library
add_library(simulator_lib STATIC ${SIMULATOR_SOURCES})
target_include_directories(simulator_lib PUBLIC
  # Our boost compatibility headers first to override others
  ${CMAKE_CURRENT_SOURCE_DIR}/include/simulator/boost
  # Then our main include directory
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  # painlessMesh test includes (for fake_serial.hpp etc)
  ${PAINLESSMESH_PATH}/test/include
  ${PAINLESSMESH_PATH}/test/boost
  ${PAINLESSMESH_PATH}/test/TaskScheduler/src
  ${PAINLESSMESH_PATH}/test/ArduinoJson/src
  # painlessMesh library headers
  ${PAINLESSMESH_PATH}/src
  ${PAINLESSMESH_PATH}/src/arduino
  ${PAINLESSMESH_PATH}/src/boost
)
target_link_libraries(simulator_lib
  PUBLIC
    Boost::system
    Threads::Threads
    yaml-cpp
  PRIVATE
    painlessmesh
)

# Main simulator executable
add_executable(painlessmesh-simulator
  src/main.cpp
  src/cli/cli_parser.cpp
)
target_link_libraries(painlessmesh-simulator
  PRIVATE
    simulator_lib
    Boost::program_options
)

# Testing
if(ENABLE_TESTING)
  enable_testing()
  
  # Add Catch2
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
  )
  FetchContent_MakeAvailable(Catch2)
  
  # Test sources
  add_executable(simulator_tests
    test/test_virtual_node.cpp
    test/test_node_manager.cpp
    test/test_config_loader.cpp
    test/test_cli_parser.cpp
    src/cli/cli_parser.cpp
    # test/test_scenario_engine.cpp
  )
  target_link_libraries(simulator_tests
    PRIVATE
      simulator_lib
      Catch2::Catch2WithMain
      Boost::program_options
  )
  
  add_test(NAME simulator_tests COMMAND simulator_tests)
  
  message(STATUS "Testing enabled with Catch2")
endif()

# Benchmarks
if(ENABLE_BENCHMARKS)
  message(STATUS "Benchmarks enabled (to be implemented)")
  # Add benchmark executables
endif()

# Examples
if(BUILD_EXAMPLES)
  message(STATUS "Examples will be built (to be implemented)")
  # add_subdirectory(examples)
endif()

# Documentation
if(BUILD_DOCS)
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    
    if(EXISTS ${DOXYGEN_IN})
      configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
      
      add_custom_target(docs ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
      )
    else()
      message(STATUS "Doxyfile.in not found, documentation generation skipped")
    endif()
  else()
    message(STATUS "Doxygen not found, documentation cannot be built")
  endif()
endif()

# Installation
# install(TARGETS painlessmesh-simulator
#   RUNTIME DESTINATION bin
# )
# install(DIRECTORY include/
#   DESTINATION include
#   FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
# )
# install(DIRECTORY examples/
#   DESTINATION share/painlessmesh-simulator/examples
#   PATTERN "*.yaml"
# )

# Print configuration summary
message(STATUS "")
message(STATUS "=== painlessMesh Simulator Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  Testing: ${ENABLE_TESTING}")
message(STATUS "  Benchmarks: ${ENABLE_BENCHMARKS}")
message(STATUS "  Coverage: ${ENABLE_COVERAGE}")
message(STATUS "  Examples: ${BUILD_EXAMPLES}")
message(STATUS "  Documentation: ${BUILD_DOCS}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  Boost: ${Boost_VERSION}")
message(STATUS "  yaml-cpp: ${yaml-cpp_FOUND}")
message(STATUS "  ncurses: ${CURSES_FOUND}")
message(STATUS "")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "==========================================")
message(STATUS "")
