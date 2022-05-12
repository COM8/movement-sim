function(vulkan_compile_shader)
    find_program(GLS_LANG_VALIDATOR_PATH NAMES glslangValidator)
    if(${GLS_LANG_VALIDATOR_PATH})
        message(FATAL_ERROR "glslangValidator not found.")
        return()
    endif()

    cmake_parse_arguments(SHADER_COMPILE "" "INFILE;OUTFILE" "" ${ARGN})
    set(SHADER_COMPILE_INFILE_FULL "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_COMPILE_INFILE}")
    set(SHADER_COMPILE_OUTFILE_FULL "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_COMPILE_OUTFILE}")
    
    add_custom_command(OUTPUT "${SHADER_COMPILE_OUTFILE_FULL}"
                       COMMAND "${GLS_LANG_VALIDATOR_PATH}"
                       ARGS "-V"
                            "-o"
                            "${SHADER_COMPILE_OUTFILE_FULL}"
                            "--target-env"
                            "vulkan1.2"
                            "${SHADER_COMPILE_INFILE_FULL}"
                       COMMENT "Compile vulkan compute shader from file '${SHADER_COMPILE_INFILE_FULL}' to '${SHADER_COMPILE_OUTFILE_FULL}'."
                       MAIN_DEPENDENCY "${SHADER_COMPILE_INFILE_FULL}")
endfunction()
