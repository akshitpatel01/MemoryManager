cmake_minimum_required(VERSION 3.10)                         
project(hash LANGUAGES CXX C VERSION 1.0.0 DESCRIPTION "Test")

set (CMAKE_BINARY_DIR ${CMAKE_HOME_DIRECTORY}/build)
set (CMAKE_INCLUDE_PATH ${CMAKE_HOME_DIRECTORY}/include)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(libs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MONGOCXX REQUIRED IMPORTED_TARGET libmongocxx)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Werror -DCMAKE_EXPORT_COMPILE_COMMANDS")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror -DCMAKE_EXPORT_COMPILE_COMMANDS")
set (CMAKE_BUILD_TYPE Debug)
set (SOURCE_DIR src/)

file(GLOB SRC ${SOURCE_DIR}/**) 

add_library(library_node_manager STATIC "")
target_include_directories(library_node_manager
                            PRIVATE
                            PUBLIC
                            ${MONGOCXX_INCLUDE_DIRS}
                            ${CMAKE_INCLUDE_PATH}
                            libs/hash/include
                          )
target_sources(library_node_manager 
                PRIVATE
                PUBLIC
                    ${SRC}
                    ${CMAKE_INCLUDE_PATH}
              )
# test

set(exec node_manager)
add_executable(${exec}
                Main.cpp)
target_link_libraries(${exec}
                        PRIVATE
                        ${MONGOCXX_LIBRARIES}
                        library_hash
                        library_node_manager
                    )
