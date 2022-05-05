cmake_minimum_required(VERSION 3.16)

add_subdirectory(widgets)
add_subdirectory(windows)
add_subdirectory(resources)

add_library(ui STATIC UiContext.hpp UiContext.cpp)

# Propagate the variables upwards
set(RESOURCE_DEF ${RESOURCE_DEF} PARENT_SCOPE)
set(RESOURCE_DIR ${RESOURCE_DIR} PARENT_SCOPE)
set(RESOURCE_DEPS ${RESOURCE_DEPS} PARENT_SCOPE)

# ${ADWAITA_LIBRARIES}
target_link_libraries(ui PUBLIC PkgConfig::GTKMM ui_windows
                         PRIVATE logger)
