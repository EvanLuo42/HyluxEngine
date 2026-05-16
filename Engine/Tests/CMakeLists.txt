find_package(doctest CONFIG REQUIRED)

add_subdirectory(Runtime)

if (HYLUX_BUILD_EDITOR)
    add_subdirectory(Editor)
endif ()
