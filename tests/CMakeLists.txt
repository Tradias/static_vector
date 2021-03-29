project(tests)

add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE ml::static_vector)
