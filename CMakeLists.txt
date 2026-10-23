cmake_minimum_required(VERSION 3.20)
project(cpp-bench-lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
endif()

add_library(benchlab src/harness.cpp src/report.cpp)
target_include_directories(benchlab PUBLIC include)

# Benchmarks register themselves at static-init time, so they are linked as an OBJECT library
# (a static archive would let the linker drop the unreferenced registrations).
file(GLOB BENCH_SOURCES CONFIGURE_DEPENDS benchmarks/*.cpp)
add_library(benchmarks OBJECT ${BENCH_SOURCES})
target_link_libraries(benchmarks PUBLIC benchlab)

add_executable(cpp-bench-lab src/main.cpp $<TARGET_OBJECTS:benchmarks>)
target_link_libraries(cpp-bench-lab PRIVATE benchlab)

enable_testing()
add_executable(benchlab-tests tests/test_main.cpp)
target_link_libraries(benchlab-tests PRIVATE benchlab)
add_test(NAME unit COMMAND benchlab-tests)
add_test(NAME smoke COMMAND cpp-bench-lab --quick --filter vector)
add_test(NAME list COMMAND cpp-bench-lab --list)
add_test(NAME export-json COMMAND cpp-bench-lab --quick --filter "vector sum" --format json --output results.json)
add_test(NAME export-csv COMMAND cpp-bench-lab --quick --filter "vector sum" --format csv)
set_tests_properties(export-csv PROPERTIES PASS_REGULAR_EXPRESSION "group,name,n,repetitions")
add_test(NAME bad-format COMMAND cpp-bench-lab --format xml)
set_tests_properties(bad-format PROPERTIES WILL_FAIL TRUE)
