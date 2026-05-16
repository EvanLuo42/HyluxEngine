set(HYLUX_RUNTIME_TESTS_TARGET HyluxRuntimeTests)

file(GLOB_RECURSE HYLUX_RUNTIME_TESTS_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(${HYLUX_RUNTIME_TESTS_TARGET} ${HYLUX_RUNTIME_TESTS_SOURCES})

target_link_libraries(${HYLUX_RUNTIME_TESTS_TARGET}
        PRIVATE
        Hylux::Runtime
        doctest::doctest
)

target_compile_features(${HYLUX_RUNTIME_TESTS_TARGET} PRIVATE cxx_std_20)

set_target_properties(${HYLUX_RUNTIME_TESTS_TARGET} PROPERTIES
        FOLDER "Hylux/Tests"
)

add_test(NAME HyluxRuntimeTests COMMAND ${HYLUX_RUNTIME_TESTS_TARGET})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source"
        FILES ${HYLUX_RUNTIME_TESTS_SOURCES})
