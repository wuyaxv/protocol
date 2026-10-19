cmake_minimum_required(VERSION 3.16)
project(protocol VERSION 1.1.0 LANGUAGES CXX)

# protocol - A simple header generator for network protocols (C++ edition)
# RFC-like ASCII diagrams + SVG/PNG images with color themes.

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Core library: everything except the CLI entry point.
add_library(protocol_core STATIC
  src/protocol.cpp
  src/specs.cpp
  src/theme.cpp
  src/render_text.cpp
  src/render_svg.cpp
  src/render_png.cpp
)
target_include_directories(protocol_core PUBLIC src)
target_include_directories(protocol_core SYSTEM PUBLIC third_party)

if(MSVC)
  target_compile_options(protocol_core PRIVATE /W4)
else()
  target_compile_options(protocol_core PRIVATE -Wall -Wextra)
endif()

# stb_image_write is not guaranteed to work with strict-aliasing optimizations.
if(NOT MSVC)
  set_source_files_properties(src/render_png.cpp PROPERTIES
    COMPILE_OPTIONS "-fno-strict-aliasing")
endif()

# The command-line tool.
add_executable(protocol src/main.cpp)
target_link_libraries(protocol PRIVATE protocol_core)

install(TARGETS protocol RUNTIME DESTINATION bin)

# Tests: unit tests (framework-free) + golden/integration tests (shell script).
enable_testing()
add_executable(protocol_unit_tests tests/test_unit.cpp)
target_link_libraries(protocol_unit_tests PRIVATE protocol_core)
add_test(NAME unit COMMAND protocol_unit_tests)
add_test(
  NAME regression
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh" "$<TARGET_FILE:protocol>"
)
