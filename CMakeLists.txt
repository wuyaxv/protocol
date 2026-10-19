cmake_minimum_required(VERSION 3.16)
project(protocol VERSION 1.0.0 LANGUAGES CXX)

# protocol - A simple header generator for network protocols (C++ edition)
# RFC-like ASCII diagrams + SVG/PNG images with color themes.

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(protocol
  src/main.cpp
  src/protocol.cpp
  src/specs.cpp
  src/theme.cpp
  src/render_text.cpp
  src/render_svg.cpp
  src/render_png.cpp
)

target_include_directories(protocol PRIVATE src)
target_include_directories(protocol SYSTEM PRIVATE third_party)

if(MSVC)
  target_compile_options(protocol PRIVATE /W4)
else()
  target_compile_options(protocol PRIVATE -Wall -Wextra)
endif()

# stb_image_write is not guaranteed to work with strict-aliasing optimizations.
if(NOT MSVC)
  set_source_files_properties(src/render_png.cpp PROPERTIES
    COMPILE_OPTIONS "-fno-strict-aliasing")
endif()

install(TARGETS protocol RUNTIME DESTINATION bin)

enable_testing()
add_test(
  NAME regression
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh" "$<TARGET_FILE:protocol>"
)
