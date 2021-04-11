cmake_minimum_required(VERSION 3.18.0)
project(static_vector
  VERSION 0.1.0
  DESCRIPTION "C++20 random access container with compile time known size."
  HOMEPAGE_URL "https://github.com/malorubiuz/static_vector"
  LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# This is set to false when built as a subproject using add_subdirectory.
set (MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MAIN_PROJECT ON)
endif()

# Library target
add_library(static_vector INTERFACE)
target_compile_features(static_vector INTERFACE cxx_std_20)
target_include_directories(static_vector INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)
target_include_directories(static_vector SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/>)

# Alias of the library target
add_library(ml::static_vector ALIAS static_vector)

# Testing
option(ML_STATIC_VECTOR_ENABLE_TESTS "Compile constexpr tests of static_vector." ${MAIN_PROJECT})
if(ML_STATIC_VECTOR_ENABLE_TESTS)
  add_subdirectory(tests)
endif()

# Single header generation
option(ML_STATIC_VECTOR_ENABLE_SINGLE_HEADER "Generate single header using python script." ${MAIN_PROJECT})
if(ML_STATIC_VECTOR_ENABLE_SINGLE_HEADER)
  add_subdirectory(single_header)
endif()

# --- Package
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/static_vector-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/static_vector-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/static_vector)

# --- Install
install(TARGETS static_vector
  EXPORT static_vector_Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ml
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT static_vector_Targets
  FILE static_vector-targets.cmake
  NAMESPACE ml::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/static_vector)

# --- Exports
export(EXPORT static_vector_Targets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/static_vector/static_vector-targets.cmake"
  NAMESPACE ml::)
