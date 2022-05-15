cmake_minimum_required(VERSION 3.16)

add_library(utils TickDurationHistory.cpp
                  TickDurationHistory.hpp
                  TickRate.cpp
                  TickRate.hpp)

target_link_libraries(utils PRIVATE fmt::fmt)
