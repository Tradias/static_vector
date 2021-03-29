cmake_minimum_required(VERSION 3.18.0)
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
option(ML_ENABLE_STATIC_VECTOR_TESTING "Compile constexpr tests of static_vector." ON)

if(ML_ENABLE_STATIC_VECTOR_TESTING)
  add_subdirectory(tests)
endif()

# Single header generation
option(ML_ENABLE_SINGLE_HEADER_GENERATION "Generate single header using python script." ON)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
  message("Python3 found, adding single header generation target.")

  add_custom_target(single_header ALL
    VERBATIM
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_LIST_DIR}/scripts/single_header.py
      -I ${CMAKE_CURRENT_LIST_DIR}/include
      --include-match=ml/*
      -o ${CMAKE_CURRENT_LIST_DIR}/single_header/static_vector.hpp
      ${CMAKE_CURRENT_LIST_DIR}/include/ml/static_vector/static_vector.hpp
    COMMENT Generate single header.)
else()
  message("Python3 not found, can't add single header generation target.")
endif()
