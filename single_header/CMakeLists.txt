
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
  add_custom_target(single_header ALL
    VERBATIM
    COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/scripts/single_header.py
      -I ${PROJECT_SOURCE_DIR}/include
      --include-match=ml/*
      -o ${CMAKE_CURRENT_LIST_DIR}/include/static_vector/static_vector.hpp
      ${PROJECT_SOURCE_DIR}/include/ml/static_vector/static_vector.hpp
    COMMENT Generate single header.)
else()
  message("Python3 not found, can't add single header target.")
endif()
