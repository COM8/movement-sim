cmake_minimum_required(VERSION 3.16)

add_library(ui_windows MainWindow.hpp MainWindow.cpp)

target_link_libraries(ui_windows PRIVATE PkgConfig::GTKMM logger ui_widgets)
