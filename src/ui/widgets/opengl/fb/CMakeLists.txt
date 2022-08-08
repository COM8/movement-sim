cmake_minimum_required(VERSION 3.16)

add_library(ui_widgets_opengl_fb AbstractGlFrameBuffer.hpp
                                 AbstractGlFrameBuffer.cpp
                                 EntitiesFrameBuffer.hpp
                                 EntitiesFrameBuffer.cpp
                                 MapFrameBuffer.hpp
                                 MapFrameBuffer.cpp
                                 QuadTreeGridFrameBuffer.hpp
                                 QuadTreeGridFrameBuffer.cpp)

target_link_libraries(ui_widgets_opengl_fb PRIVATE logger PkgConfig::EPOXY PkgConfig::GTKMM ui_widgets_opengl_utils)
