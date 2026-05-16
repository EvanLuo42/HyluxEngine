find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui)

set(HYLUX_EDITOR_TESTS_TARGET HyluxEditorTests)

file(GLOB_RECURSE HYLUX_EDITOR_TESTS_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(${HYLUX_EDITOR_TESTS_TARGET} ${HYLUX_EDITOR_TESTS_SOURCES})

target_link_libraries(${HYLUX_EDITOR_TESTS_TARGET}
        PRIVATE
        Hylux::EditorLib
        Qt6::Core
        Qt6::Gui
        doctest::doctest
)

target_compile_features(${HYLUX_EDITOR_TESTS_TARGET} PRIVATE cxx_std_20)

set_target_properties(${HYLUX_EDITOR_TESTS_TARGET} PROPERTIES
        FOLDER "Hylux/Tests"
)

add_test(NAME HyluxEditorTests COMMAND ${HYLUX_EDITOR_TESTS_TARGET})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source"
        FILES ${HYLUX_EDITOR_TESTS_SOURCES})
