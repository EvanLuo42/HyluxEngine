find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Widgets)
find_package(KDDockWidgets-qt6 CONFIG REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)

qt_policy(SET QTP0004 NEW)

set(HYLUX_EDITOR_TARGET HyluxEditor)

file(GLOB_RECURSE HYLUX_EDITOR_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
)

file(GLOB_RECURSE HYLUX_EDITOR_QML_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.qml"
)

foreach (QML_FILE ${HYLUX_EDITOR_QML_FILES})
    file(RELATIVE_PATH QML_ALIAS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/qml ${QML_FILE})
    set_source_files_properties(${QML_FILE} PROPERTIES QT_RESOURCE_ALIAS ${QML_ALIAS})
endforeach ()

qt_add_executable(${HYLUX_EDITOR_TARGET} WIN32
        ${HYLUX_EDITOR_SOURCES}
)

qt_add_qml_module(${HYLUX_EDITOR_TARGET}
        URI Hylux.Editor
        VERSION 1.0
        QML_FILES ${HYLUX_EDITOR_QML_FILES}
        RESOURCE_PREFIX /qt/qml
)

target_link_libraries(${HYLUX_EDITOR_TARGET}
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        KDAB::kddockwidgets
        Hylux::Runtime
)

set_target_properties(${HYLUX_EDITOR_TARGET} PROPERTIES
        FOLDER "Hylux"
        WIN32_EXECUTABLE ON
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${HYLUX_EDITOR_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Resources PREFIX "Resources" FILES ${HYLUX_EDITOR_QML_FILES})

if (WIN32)
    find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
    add_custom_command(TARGET ${HYLUX_EDITOR_TARGET} POST_BUILD
            COMMAND ${WINDEPLOYQT}
            --no-translations
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/Resources/qml"
            "$<TARGET_FILE:${HYLUX_EDITOR_TARGET}>"
            VERBATIM
    )
endif()
