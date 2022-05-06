cmake_minimum_required(VERSION 3.16)

add_library(ui_widgets STATIC SimulationWidget.cpp
                              SimulationWidget.hpp
                              SimulationSettingsBarWidget.cpp
                              SimulationSettingsBarWidget.hpp)

target_link_libraries(ui_widgets PRIVATE PkgConfig::GTKMM logger sim)
