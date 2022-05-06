cmake_minimum_required(VERSION 3.20)

vulkan_compile_shader(INFILE random_move.comp
                      OUTFILE random_move.hpp
                      NAME random_move)

add_library(sim_shaders STATIC "${CMAKE_CURRENT_BINARY_DIR}/random_move.hpp")
set_target_properties(sim_shaders PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(sim_shaders PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
