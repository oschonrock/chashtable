cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER clang)

project(projects)
set(CMAKE_C_STANDARD 11) # C11 standard

# santiser
set(CMAKE_C_FLAGS_DEBUG            -fsanitize=address,undefined)
set(CMAKE_C_FLAGS_RELWITHDEBINFO   -fsanitize=address,undefined)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) 

add_subdirectory(Unity)

include_directories(include)

add_library(libhashtable src/hashtable.c)

add_executable(hashtable  apps/hashtable.c)
target_link_libraries(hashtable PRIVATE libhashtable)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests) 

add_executable(test_hashtable  tests/test_hashtable.c)
target_include_directories(test_hashtable PRIVATE Unity/src)
target_link_libraries(test_hashtable PRIVATE unity libhashtable)
