cmake_minimum_required(VERSION 3.16)

add_subdirectory(shader)

add_library(sim Simulator.cpp
                Simulator.hpp
                Entity.cpp
                Entity.hpp
                Map.cpp
                Map.hpp
                PushConsts.cpp
                PushConsts.hpp
                GpuQuadTree.cpp
                GpuQuadTree.hpp)

target_link_libraries(sim PRIVATE kompute::kompute logger sim_shader utils nlohmann_json::nlohmann_json ${CMAKE_DL_LIBS})
