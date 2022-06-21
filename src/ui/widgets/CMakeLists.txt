cmake_minimum_required(VERSION 3.16)

add_library(ui_widgets SimulationWidget.cpp
                              SimulationWidget.hpp
                              SimulationOverlayWidget.cpp
                              SimulationOverlayWidget.hpp
                              SimulationSettingsBarWidget.cpp
                              SimulationSettingsBarWidget.hpp)

target_link_libraries(ui_widgets PRIVATE PkgConfig::GTKMM logger sim PkgConfig::EPOXY fmt::fmt utils ui_widgets_opengl ui_widgets_opengl_fb)

add_subdirectory(opengl)
