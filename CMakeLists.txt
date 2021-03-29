cmake_minimum_required(VERSION 3.12.0)
project(static_vector
  VERSION 0.1.0
  DESCRIPTION "C++20 random access container with compile time known size."
  HOMEPAGE_URL "https://github.com/malorubiuz/static_vector"
  LANGUAGES CXX)

# Library interface target
add_library(static_vector INTERFACE)
target_compile_features(static_vector INTERFACE cxx_std_20)
target_include_directories(static_vector INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

# Alias of the library target
add_library(ml::static_vector ALIAS static_vector)

# Testing
option(ML_ENABLE_STATIC_VECTOR_TESTING "Compile constexpr tests of static_vector" ON)

if(ML_ENABLE_STATIC_VECTOR_TESTING)
  add_subdirectory(tests)
endif()
