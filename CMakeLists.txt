cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER clang)

project(projects)
set(CMAKE_C_STANDARD 11) # C11 standard
# set(CMAKE_C_EXTENSIONS OFF) # no gnu extensions

# santiser
set(CMAKE_C_FLAGS_DEBUG            -fsanitize=address,undefined)
set(CMAKE_C_FLAGS_RELWITHDEBINFO   -fsanitize=address,undefined)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# set(CMAKE_C_FLAGS "-D_GNU_SOURCE ${CMAKE_C_FLAGS}") # make strdup work
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) 

include_directories(include)
add_executable(hashtable  apps/hashtable.c src/hashtable.c)

