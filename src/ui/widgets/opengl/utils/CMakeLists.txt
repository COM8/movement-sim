cmake_minimum_required(VERSION 3.16)

add_library(ui_widgets_opengl_utils Utils.hpp
                                 Utils.cpp)

target_link_libraries(ui_widgets_opengl_utils PRIVATE logger PkgConfig::EPOXY)
