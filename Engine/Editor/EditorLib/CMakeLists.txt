set(HYLUX_EDITOR_LIB_TARGET HyluxEditorLib)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
find_package(KDDockWidgets-qt6 CONFIG REQUIRED)
find_package(slang CONFIG REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)

file(GLOB_RECURSE HYLUX_EDITOR_LIB_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

add_library(${HYLUX_EDITOR_LIB_TARGET} STATIC ${HYLUX_EDITOR_LIB_SOURCES})

set_target_properties(${HYLUX_EDITOR_LIB_TARGET} PROPERTIES
        FOLDER "Hylux"
        DEBUG_POSTFIX "d"
        AUTOMOC ON
)

target_include_directories(${HYLUX_EDITOR_LIB_TARGET}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source>
)

target_compile_features(${HYLUX_EDITOR_LIB_TARGET} PUBLIC cxx_std_20)

target_compile_definitions(${HYLUX_EDITOR_LIB_TARGET} PUBLIC QT_NO_KEYWORDS)

target_link_libraries(${HYLUX_EDITOR_LIB_TARGET}
        PUBLIC
        Hylux::Runtime
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        KDAB::kddockwidgets
        PRIVATE
        slang::slang
)

add_library(Hylux::EditorLib ALIAS ${HYLUX_EDITOR_LIB_TARGET})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source"
        FILES ${HYLUX_EDITOR_LIB_SOURCES})
