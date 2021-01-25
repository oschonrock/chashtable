cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER clang)

project(topwords)
set(CMAKE_C_STANDARD 11) # C11 standard


# defaults from cmake
# -DCMAKE_BUILD_TYPE=Debug              adds           -g
# -DCMAKE_BUILD_TYPE=MinSizeRel         adds       -Os    -DNDEBUG
# -DCMAKE_BUILD_TYPE=RelWithDebInfo     adds       -O2 -g -DNDEBUG
# -DCMAKE_BUILD_TYPE=Release            adds       -O3    -DNDEBUG

# santiser
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD") # FreeBSD hangs with sanitizer
  set(CMAKE_C_FLAGS_DEBUG            -fsanitize=address,undefined)

  # use this to detect unitialized memory
  # set(CMAKE_C_FLAGS_DEBUG            -fsanitize=memory,undefined)

# set(CMAKE_C_FLAGS_RELWITHDEBINFO   -fsanitize=address,undefined) // for use with valgrind instead
endif()  
set(CMAKE_C_FLAGS_DEBUG            -fsanitize=address,undefined)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) 

add_subdirectory(Unity)
include_directories(include)

add_library(hashtable src/hashtable.c)

add_executable(topwords  apps/topwords.c)
target_link_libraries(topwords PRIVATE hashtable)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests) 

add_executable(test_hashtable  tests/test_hashtable.c)
target_include_directories(test_hashtable PRIVATE src Unity/src)
target_link_libraries(test_hashtable PRIVATE unity hashtable)
