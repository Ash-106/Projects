cmake_minimum_required(VERSION 3.16)
project(mc_risk_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

find_package(Threads REQUIRED)

add_library(mcre INTERFACE)
target_include_directories(mcre INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(mcre INTERFACE Threads::Threads)

add_executable(mcre_cli src/main.cpp)
target_link_libraries(mcre_cli PRIVATE mcre)
set_target_properties(mcre_cli PROPERTIES OUTPUT_NAME mcre)

enable_testing()
add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE mcre)
add_test(NAME core COMMAND test_core)
