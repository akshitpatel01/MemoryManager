cmake_minimum_required(VERSION 3.10)                         
project(hash LANGUAGES CXX C VERSION 1.0.0 DESCRIPTION "Test")


set (CMAKE_SOURCE_DIR ${CMAKE_HOME_DIRECTORY}/src)
set (CMAKE_BINARY_DIR ${CMAKE_HOME_DIRECTORY}/build)
set (CMAKE_INCLUDE_PATH ${CMAKE_HOME_DIRECTORY}/include)
#set (SRC ${CMAKE_SOURCE_DIR}/hash_linear.cpp ${CMAKE_SOURCE_DIR}/util.c)
file(GLOB SRC ${CMAKE_SOURCE_DIR}/**) 
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Werror -DCMAKE_EXPORT_COMPILE_COMMANDS")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror -DCMAKE_EXPORT_COMPILE_COMMANDS")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories (${CMAKE_INCLUDE_PATH})
set(CMAKE_BUILD_TYPE Debug)

add_library(hash_lib STATIC ${SRC})
target_include_directories(hash_lib PUBLIC ${CMAKE_INCLUDE_PATH})


# test

set(exec test)

add_executable(${exec}
                Main.cpp )
target_link_libraries(${exec}
                        hash_lib)
