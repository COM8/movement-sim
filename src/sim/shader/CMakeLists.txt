cmake_minimum_required(VERSION 3.20)

vulkan_compile_shader(INFILE fall.comp
                      OUTFILE fall.hpp
                      NAMESPACE "sim")

vulkan_compile_shader(INFILE random_move.comp
                      OUTFILE random_move.hpp
                      NAMESPACE "sim")

add_library(sim_shader "${CMAKE_CURRENT_BINARY_DIR}/fall.hpp"
                       "${CMAKE_CURRENT_BINARY_DIR}/random_move.hpp")

set_target_properties(sim_shader PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(sim_shader PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

add_subdirectory(utils)
