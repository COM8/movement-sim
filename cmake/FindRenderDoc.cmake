find_path(RENDERDOC_INCLUDE_DIRS renderdoc_app.h REQUIRED)

find_library(RENDERDOC_LIBRARY NAMES renderdoc
                               REQUIRED
                               HINTS "/usr/lib64/renderdoc")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RenderDoc DEFAULT_MSG RENDERDOC_INCLUDE_DIRS RENDERDOC_LIBRARY)

mark_as_advanced(RENDERDOC_INCLUDE_DIRS RENDERDOC_LIBRARY)
