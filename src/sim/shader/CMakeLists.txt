cmake_minimum_required(VERSION 3.20)

vulkan_compile_shader(INFILE random_move_vec.comp
                      OUTFILE random_move_vec.spv)

vulkan_compile_shader(INFILE random_move_struct.comp
                      OUTFILE random_move_struct.spv)

add_custom_target(sim_shader ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/random_move_vec.spv"
                                         "${CMAKE_CURRENT_BINARY_DIR}/random_move_struct.spv")

add_subdirectory(utils)
