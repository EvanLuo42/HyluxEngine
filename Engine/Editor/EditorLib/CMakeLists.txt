set(HYLUX_EDITOR_LIB_TARGET HyluxEditorLib)

file(GLOB_RECURSE HYLUX_EDITOR_LIB_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

add_library(${HYLUX_EDITOR_LIB_TARGET} STATIC ${HYLUX_EDITOR_LIB_SOURCES})

target_include_directories(${HYLUX_EDITOR_LIB_TARGET}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source>
)

target_compile_features(${HYLUX_EDITOR_LIB_TARGET} PUBLIC cxx_std_20)

find_package(slang CONFIG REQUIRED)

target_link_libraries(${HYLUX_EDITOR_LIB_TARGET}
        PUBLIC
        Hylux::Runtime
        PRIVATE
        slang::slang
)

set_target_properties(${HYLUX_EDITOR_LIB_TARGET} PROPERTIES
        FOLDER "Hylux"
        DEBUG_POSTFIX "d"
)

add_library(Hylux::EditorLib ALIAS ${HYLUX_EDITOR_LIB_TARGET})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source"
        FILES ${HYLUX_EDITOR_LIB_SOURCES})
