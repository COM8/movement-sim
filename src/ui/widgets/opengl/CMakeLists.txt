cmake_minimum_required(VERSION 3.16)

add_library(ui_widgets_opengl AbstractGlObject.hpp
                              AbstractGlObject.cpp
                              EntityGlObject.hpp
                              EntityGlObject.cpp
                              MapGlObject.hpp
                              MapGlObject.cpp
                              ScreenSquareGlObject.hpp
                              ScreenSquareGlObject.cpp
                              BlurGlObject.hpp
                              BlurGlObject.cpp)

target_link_libraries(ui_widgets_opengl PRIVATE logger PkgConfig::EPOXY PkgConfig::GTKMM ui_widgets_opengl_utils)

add_subdirectory(utils)
add_subdirectory(fb)
