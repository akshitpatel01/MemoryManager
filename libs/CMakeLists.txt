cmake_minimum_required(VERSION 3.13.0)                         
project(libs LANGUAGES CXX C VERSION 1.0.0 DESCRIPTION "Test")

add_subdirectory(hash)
#add_library(library_node_manager STATIC "")
#message(${HASH_INC_DIRS})
#target_include_directories(library_node_manager
#                            PRIVATE
#                            ${CMAKE_INCLUDE_PATH}
#                            ${MONGOCXX_INCLUDE_DIRS}
#                            ${HASH_INC_DIRS}
#                            include/
#                          )
                      #target_sources(library_node_manager 
                      #                PRIVATE
                      #                    ${SRC}
                      #                PUBLIC
                      #                    include/node_manager.h
                      #              )
