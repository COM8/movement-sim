cmake_minimum_required(VERSION 3.20)

# Fix install RPATH for Linux:
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

set(MAIN_EXECUTABLE ${PROJECT_NAME})

add_subdirectory(logger)
add_subdirectory(utils)
add_subdirectory(sim)
add_subdirectory(ui)

glib_add_resource_file(TARGET ui_resources
                       RESOURCEDEF ${RESOURCE_DEF}
                       WORKING_DIR ${RESOURCE_DIR}
                       OUTFILE ui_resources.c
                       DEPENDS ${RESOURCE_DEPS})

add_executable(${MAIN_EXECUTABLE} main.cpp ${CMAKE_CURRENT_BINARY_DIR}/ui_resources.c)

target_link_libraries(${MAIN_EXECUTABLE} PRIVATE logger ui sim PkgConfig::GTKMM)
set_property(SOURCE main.cpp PROPERTY COMPILE_DEFINITIONS MOVEMENT_SIMULATOR_VERSION="${PROJECT_VERSION}" MOVEMENT_SIMULATOR_VERSION_NAME="${VERSION_NAME}")

install(TARGETS ${MAIN_EXECUTABLE} RUNTIME DESTINATION)
