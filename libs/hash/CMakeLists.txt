cmake_minimum_required(VERSION 3.13.0)                         
project(hash LANGUAGES CXX C VERSION 1.0.0 DESCRIPTION "Test")


set (BINARY_DIR build)
set (SOURCE_DIR src)
set (INC_DIRS include/)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Werror -DCMAKE_EXPORT_COMPILE_COMMANDS")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror -DCMAKE_EXPORT_COMPILE_COMMANDS")
set (CMAKE_BUILD_TYPE Release)

file(GLOB SRC ${SOURCE_DIR}/**) 
add_library(library_hash STATIC "")
target_include_directories(library_hash 
                            PRIVATE
                            PUBLIC
                            ${INC_DIRS}
                           )
target_sources(library_hash 
                PRIVATE
                PUBLIC
                    ${SRC}
                    ${INC_DIRS}
                )
