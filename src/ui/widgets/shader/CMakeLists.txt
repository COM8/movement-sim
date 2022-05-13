cmake_minimum_required(VERSION 3.20)

copy_shader_file(FILE "entity_draw.frag")

add_custom_target(ui_widgets_shader ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/entity_draw.frag")
