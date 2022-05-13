cmake_minimum_required(VERSION 3.20)

vulkan_compile_shader(INFILE fall.comp
                      OUTFILE fall.spv)

vulkan_compile_shader(INFILE random_move.comp
                      OUTFILE random_move.spv)

add_custom_target(sim_shader ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/fall.spv"
                                         "${CMAKE_CURRENT_BINARY_DIR}/random_move.spv")

add_subdirectory(utils)
