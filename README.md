# static_vector
C++20 implementation of a std::vector-like container with a compile time known size, stored on the stack.

## documentation
Documentation is provided as source code comments and on [docsforge](https://static-vector.docsforge.com/main/).

## use as subproject using CMake
add_subdirectory(static_vector)

target_link_libraries(your_project PRIVATE ml::static_vector)

## use as single header file
include the single header provided in single_header/include/

## examples
Basic example on godbolt: https://godbolt.org/z/MaadqbhGa
