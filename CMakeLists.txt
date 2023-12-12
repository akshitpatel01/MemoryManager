cmake_minimum_required(VERSION 3.10)                         
project(Blob_manager LANGUAGES CXX C VERSION 1.0.0 DESCRIPTION "Test")

set (CMAKE_BINARY_DIR ${CMAKE_HOME_DIRECTORY}/build)
set (CMAKE_INCLUDE_PATH ${CMAKE_HOME_DIRECTORY}/include)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(libs)
add_subdirectory(rpc)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Werror -DCMAKE_EXPORT_COMPILE_COMMANDS")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror -DCMAKE_EXPORT_COMPILE_COMMANDS")
set (CMAKE_BUILD_TYPE Debug)
set (SOURCE_DIR src/)

file(GLOB SRC ${SOURCE_DIR}/**) 

add_library(library_blob_manager STATIC "")
target_include_directories(library_blob_manager
                            PRIVATE
                            PUBLIC
                            ${CMAKE_INCLUDE_PATH}
                            libs/hash/include
                            ${ProtobufIncludePath} 
                            ${GRPC_INC_DIRS}
                          )
target_sources(library_blob_manager 
                PRIVATE
                PUBLIC
                    ${SRC}
                    ${CMAKE_INCLUDE_PATH}
                    ${GRPC_INC_DIRS}
              )
target_link_libraries(library_blob_manager
                        PUBLIC
                        reg_proto_lib
                        )
# test

set(exec blob_manager)
add_executable(${exec}
                Main.cpp)
target_link_libraries(${exec}
                        PRIVATE
                        library_hash
                        library_blob_manager
                    )
