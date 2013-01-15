cmake_minimum_required(VERSION 2.8)

set(Boost_FIND_REQUIRED 1)
if (BOOST_ROOT)
  # Prevent falling back to system paths when using a custom Boost prefix.
  set(Boost_NO_SYSTEM_PATHS true)
endif ()

find_package(Boost 1.51.0 COMPONENTS
    unit_test_framework)
if (NOT Boost_FOUND)
    message(FATAL_ERROR, "Boost >= 1.51 not found")
else()
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++11 -ggdb")
add_library(store hashstore.cc termstore.cc)

add_definitions(-DBOOST_TEST_DYN_LINK)
add_executable(test_mmap test_mmap.cc)
target_link_libraries(test_mmap boost_unit_test_framework)
