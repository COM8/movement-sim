cmake_minimum_required(VERSION 3.20)

set(RESOURCE_DEF ${CMAKE_CURRENT_SOURCE_DIR}/ui_resources.c.xml PARENT_SCOPE)
set(RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(RESOURCE_DEPS ${CMAKE_CURRENT_SOURCE_DIR}/theme.css

    ${CMAKE_CURRENT_SOURCE_DIR}/shader/entity/entity.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/entity/entity.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/entity/entity.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/screen_square/screen_square.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/screen_square/screen_square.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/screen_square/screen_square.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/map/map.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/map/map.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/blur/blur.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/blur/blur.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/blur/blur.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/quadTreeGrid/quadTreeGrid.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader/quadTreeGrid/quadTreeGrid.vert

    ${CMAKE_CURRENT_SOURCE_DIR}/icons/scalable/action/closed-captioning-symbolic.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/scalable/action/display-with-window-symbolic.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/scalable/action/pause-large-symbolic.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/scalable/action/pencil-symbolic.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/scalable/action/play-large-symbolic.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/scalable/action/test-symbolic.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/scalable/action/blur-symbolic.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/icons/scalable/action/transparent-background-symbolic.svg PARENT_SCOPE)
