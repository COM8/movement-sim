function(copy_shader_file)

    cmake_parse_arguments(COPY_SHADER "" "FILE" "" ${ARGN})
    set(COPY_SHADER_INFILE_FULL "${CMAKE_CURRENT_SOURCE_DIR}/${COPY_SHADER_FILE}")
    set(COPY_SHADER_OUTFILE_FULL "${CMAKE_CURRENT_BINARY_DIR}/${COPY_SHADER_FILE}")
    
    add_custom_command(OUTPUT "${COPY_SHADER_OUTFILE_FULL}"
                       POST_BUILD
                       COMMAND ${CMAKE_COMMAND}
                       ARGS "-E"
                            "copy"
                            "${COPY_SHADER_INFILE_FULL}"
                            "${COPY_SHADER_OUTFILE_FULL}"
                       COMMENT "Copying shader from '${COPY_SHADER_INFILE_FULL}' to '${COPY_SHADER_OUTFILE_FULL}'."
                       MAIN_DEPENDENCY "${COPY_SHADER_INFILE_FULL}")
endfunction()
