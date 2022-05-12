cmake_minimum_required(VERSION 3.16)

add_subdirectory(shader)

add_library(sim Simulator.cpp
                Simulator.hpp)

target_link_libraries(sim PRIVATE kompute::kompute logger sim_shaders_utils)
add_dependencies(sim sim_shader)
