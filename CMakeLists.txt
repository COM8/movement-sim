cmake_minimum_required(VERSION 3.20)

set(PROJECT_DESCRIPTION "GPU accelerated human movement simulator.")
project("mSim" 
            VERSION 0.1.0
            DESCRIPTION "${PROJECT_DESCRIPTION}"
            HOMEPAGE_URL "https://github.com/COM8/movement-sim.git")
set(VERSION_NAME "dev")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

macro(movement_simulator_option OPTION_NAME OPTION_TEXT OPTION_DEFAULT)
    option(${OPTION_NAME} ${OPTION_TEXT} ${OPTION_DEFAULT})
    if(DEFINED ENV{${OPTION_NAME}})
        # Allow setting the option through an environment variable
        set(${OPTION_NAME} $ENV{${OPTION_NAME}})
    endif()
    if(${OPTION_NAME})
        add_definitions(-D${OPTION_NAME})
    endif()
    message(STATUS "  ${OPTION_NAME}: ${${OPTION_NAME}}")
endmacro()

macro(movement_simulator_log_level_option OPTION_NAME OPTION_TEXT OPTION_DEFAULT)
    set(${OPTION_NAME} ${OPTION_DEFAULT} CACHE STRING ${OPTION_TEXT})
    set_property(CACHE ${OPTION_NAME} PROPERTY STRINGS "Default" "Trace" "Debug" "Info" "Warn" "Error" "Critical" "Off")
    if(DEFINED ENV{${OPTION_NAME}})
        # Allow setting the option through an environment variable
        set(${OPTION_NAME} $ENV{${OPTION_NAME}})
    endif()
    if(${OPTION_NAME})
        add_definitions(-D${OPTION_NAME})
    endif()
    message(STATUS "  ${OPTION_NAME}: ${${OPTION_NAME}}")
endmacro()

message(STATUS "Movement Simulator client options")
message(STATUS "=======================================================")
movement_simulator_option(MOVEMENT_SIMULATOR_STATIC_ANALYZE "Set to ON to enable the GCC 10 static analysis. If enabled, MOVEMENT_SIMULATOR_ENABLE_LINTING has to be disabled." OFF)
movement_simulator_option(MOVEMENT_SIMULATOR_ENABLE_LINTING "Set to ON to enable clang linting. If enabled, MOVEMENT_SIMULATOR_STATIC_ANALYZE has to be disabled." OFF)
movement_simulator_option(MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API "Set to ON to enable RenderDoc in application APIs for Kompute to allow inspecting compute shaders." OFF)
movement_simulator_log_level_option(MOVEMENT_SIMULATOR_LOG_LEVEL "Sets the log level. 'Default' will set the log level to 'Debug' for debug builds and 'Info' for release builds." "Off")
message(STATUS "=======================================================")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(sanitizer)
include(gcc_analyze)
include(clear_variable)
include(FetchContent)
include(vulkan_shader_compiler)
include(copy_shader_file)

if(MOVEMENT_SIMULATOR_ENABLE_LINTING)
    message(STATUS "Enabling linting")
    include(clang-tidy)
else()
    message(STATUS "Linting is disabled")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

# gtk4 stuff
include(glib-resource-compiler)
find_package(PkgConfig REQUIRED)
# Additional search path since gtkmm-4.0 will usually be installed in /usr
set(ENV{PKG_CONFIG_PATH} "/usr/lib/pkgconfig")
pkg_check_modules(GTKMM gtkmm-4.0>=4.4.0 REQUIRED IMPORTED_TARGET)
pkg_check_modules(EPOXY epoxy REQUIRED IMPORTED_TARGET)

# Lib adwaita
pkg_check_modules(ADWAITA REQUIRED libadwaita-1>=1.1.0)
include_directories(${ADWAITA_INCLUDE_DIRS})

if(MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API)
    # So CMake can find FindRenderDoc.cmake
    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

    message("RenderDoc in application API is enabled.")
    find_package(RenderDoc REQUIRED)
    add_compile_definitions(MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API)
endif()

# Disable linting for fetch content projects
clear_variable(DESTINATION CMAKE_CXX_CLANG_TIDY BACKUP CMAKE_CXX_CLANG_TIDY_BKP)
clear_variable(DESTINATION CMAKE_CXX_FLAGS BACKUP CMAKE_CXX_FLAGS_BKP)
clear_variable(DESTINATION CMAKE_CXX_FLAGS_DEBUG BACKUP CMAKE_CXX_FLAGS_DEBUG_BKP)

FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git   
                         GIT_TAG b6f4ceaed0a0a24ccf575fab6c56dd50ccf6f1a9) # The hash for the release commit of 8.1.1 from: https://github.com/fmtlib/fmt/releases
FetchContent_MakeAvailable(fmt)
include_directories(${fmt_SOURCE_DIR}/include)

set(SPDLOG_INSTALL OFF)
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git
                            GIT_TAG v1.9.2)
FetchContent_MakeAvailable(spdlog)

set(KOMPUTE_OPT_DISABLE_VK_DEBUG_LAYERS ON) # Version clash since they are not properly supported on my GPU.
set(KOMPUTE_OPT_USE_BUILD_IN_VULKAN_HEADER OFF)
set(KOMPUTE_OPT_BUILD_SHADERS ON)
set(KOMPUTE_OPT_LOG_LEVEL "Off")
FetchContent_Declare(kompute GIT_REPOSITORY https://github.com/COM8/kompute.git
                             GIT_TAG 53a3753caf997cee19a4c8021fafab0ae71de5d2)
FetchContent_MakeAvailable(kompute)
include_directories(${kompute_SOURCE_DIR}/src/include)

FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git
                          GIT_TAG v3.10.5)
FetchContent_MakeAvailable(json)

restore_variable(DESTINATION CMAKE_CXX_CLANG_TIDY BACKUP CMAKE_CXX_CLANG_TIDY_BKP)
restore_variable(DESTINATION CMAKE_CXX_FLAGS BACKUP CMAKE_CXX_FLAGS_BKP)
restore_variable(DESTINATION CMAKE_CXX_FLAGS_DEBUG BACKUP CMAKE_CXX_FLAGS_DEBUG_BKP)

include_directories(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(src)