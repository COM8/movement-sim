cmake_minimum_required(VERSION 3.16)

add_library(ui_widgets OBJECT MyWidget.cpp)

target_link_libraries(ui_widgets PUBLIC PkgConfig::GTKMM
                                 PRIVATE logger)
