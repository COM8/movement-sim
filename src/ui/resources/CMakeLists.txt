cmake_minimum_required(VERSION 3.20)

set(RESOURCE_DEF ${CMAKE_CURRENT_SOURCE_DIR}/ui_resources.c.xml PARENT_SCOPE)
set(RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(RESOURCE_DEPS ${CMAKE_CURRENT_SOURCE_DIR}/theme.css
                  ${CMAKE_CURRENT_SOURCE_DIR}/shader/person/person.frag
                  ${CMAKE_CURRENT_SOURCE_DIR}/shader/person/person.vert
                  ${CMAKE_CURRENT_SOURCE_DIR}/shader/person/person.geom
                  ${CMAKE_CURRENT_SOURCE_DIR}/shader/screen_square/screen_square.frag
                  ${CMAKE_CURRENT_SOURCE_DIR}/shader/screen_square/screen_square.vert
                  ${CMAKE_CURRENT_SOURCE_DIR}/shader/screen_square/screen_square.geom
                  ${CMAKE_CURRENT_SOURCE_DIR}/shader/map/map.frag
                  ${CMAKE_CURRENT_SOURCE_DIR}/shader/map/map.vert
                  ${CMAKE_CURRENT_SOURCE_DIR}/shader/map/map.geom PARENT_SCOPE)
