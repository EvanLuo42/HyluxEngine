set(HYLUX_RUNTIME_TARGET HyluxRuntime)

file(GLOB_RECURSE HYLUX_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Backend-specific subtrees are added back below when the corresponding option
# is on. Filter them out of the base list so toggling an option flips the entire
# subtree in/out of the build (and so unselected SDKs are never #include'd).
list(FILTER HYLUX_RUNTIME_SOURCES EXCLUDE REGEX
     "/RHI/(Vulkan|Capture/Nsight|Diagnostics/Aftermath)/")

set(HYLUX_RUNTIME_PCH_HEADER pch.h)

add_library(${HYLUX_RUNTIME_TARGET} STATIC ${HYLUX_RUNTIME_SOURCES})

add_library(Hylux::Runtime ALIAS ${HYLUX_RUNTIME_TARGET})

target_include_directories(${HYLUX_RUNTIME_TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(${HYLUX_RUNTIME_TARGET} PUBLIC cxx_std_20)

target_precompile_headers(${HYLUX_RUNTIME_TARGET} PRIVATE ${HYLUX_RUNTIME_PCH_HEADER})

target_compile_definitions(${HYLUX_RUNTIME_TARGET} PUBLIC
    $<$<CONFIG:Debug>:HYLUX_LOG_LEVEL_MIN=0>
    $<$<CONFIG:RelWithDebInfo>:HYLUX_LOG_LEVEL_MIN=1>
    $<$<CONFIG:Release>:HYLUX_LOG_LEVEL_MIN=2>
    $<$<CONFIG:MinSizeRel>:HYLUX_LOG_LEVEL_MIN=2>
)

if (WIN32)
    target_link_libraries(${HYLUX_RUNTIME_TARGET} PUBLIC Shell32 Ole32)
endif ()

if (HYLUX_RHI_VULKAN)
    include(FindHyluxVulkan)
    file(GLOB_RECURSE HYLUX_RHI_VULKAN_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Vulkan/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Vulkan/*.cpp")
    target_sources(${HYLUX_RUNTIME_TARGET} PRIVATE ${HYLUX_RHI_VULKAN_SOURCES})
    target_link_libraries(${HYLUX_RUNTIME_TARGET} PRIVATE Hylux::Vulkan)
    target_compile_definitions(${HYLUX_RUNTIME_TARGET} PUBLIC HYLUX_RHI_VULKAN=1)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source"
                 FILES ${HYLUX_RHI_VULKAN_SOURCES})
endif ()

if (HYLUX_CAPTURE_NSIGHT_ACTIVE)
    include(FindNsightSdks)
    if (TARGET Nvidia::NsightGraphics)
        file(GLOB_RECURSE HYLUX_CAPTURE_NSIGHT_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Capture/Nsight/*.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Capture/Nsight/*.cpp")
        target_sources(${HYLUX_RUNTIME_TARGET} PRIVATE ${HYLUX_CAPTURE_NSIGHT_SOURCES})
        target_link_libraries(${HYLUX_RUNTIME_TARGET} PRIVATE Nvidia::NsightGraphics)
        target_compile_definitions(${HYLUX_RUNTIME_TARGET} PUBLIC HYLUX_CAPTURE_NSIGHT=1)
        source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source"
                     FILES ${HYLUX_CAPTURE_NSIGHT_SOURCES})
    else ()
        message(STATUS "Hylux: HYLUX_CAPTURE_NSIGHT active but Nvidia::NsightGraphics "
                       "target not available; capture tool will return null at runtime.")
    endif ()
endif ()

if (HYLUX_CRASH_AFTERMATH_ACTIVE)
    include(FindNsightSdks)
    if (TARGET Nvidia::Aftermath)
        file(GLOB_RECURSE HYLUX_CRASH_AFTERMATH_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Diagnostics/Aftermath/*.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Diagnostics/Aftermath/*.cpp")
        target_sources(${HYLUX_RUNTIME_TARGET} PRIVATE ${HYLUX_CRASH_AFTERMATH_SOURCES})
        target_link_libraries(${HYLUX_RUNTIME_TARGET} PRIVATE Nvidia::Aftermath)
        target_compile_definitions(${HYLUX_RUNTIME_TARGET} PUBLIC HYLUX_CRASH_AFTERMATH=1)
        # Expose the absolute DLL path as a global property so any EXE that depends on the
        # runtime can copy it next to its own binary via POST_BUILD. We do NOT attach the
        # copy to HyluxRuntime: that is a STATIC library, and its POST_BUILD only fires
        # when sources recompile — if the bin/ dir is cleaned but Runtime stays cached,
        # the copy never re-runs and the EXE fails to start (missing DLL). Resolve the
        # path at configure time (get_target_property → IMPORTED_LOCATION) rather than
        # via $<TARGET_FILE:Nvidia::Aftermath>: the imported target lives in this
        # directory scope and would not be visible to genex expansion in EXE subdirs.
        get_target_property(_hylux_aftermath_dll Nvidia::Aftermath IMPORTED_LOCATION)
        set_property(GLOBAL PROPERTY HYLUX_AFTERMATH_DLL_PATH "${_hylux_aftermath_dll}")
        source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source"
                     FILES ${HYLUX_CRASH_AFTERMATH_SOURCES})
    else ()
        message(STATUS "Hylux: HYLUX_CRASH_AFTERMATH active but Nvidia::Aftermath "
                       "target not available; crash reporter will return null at runtime.")
    endif ()
endif ()

# Helper invoked by executable targets that depend (transitively) on Aftermath. Attaches
# a POST_BUILD copy_if_different to the executable so the Aftermath DLL is guaranteed to
# land next to the .exe on every build of that exe — not gated on the static runtime
# being re-linked. Idempotent: silently skips when Aftermath isn't being built.
function(hylux_copy_aftermath_dll_next_to TARGET_NAME)
    get_property(_aftermath_dll GLOBAL PROPERTY HYLUX_AFTERMATH_DLL_PATH)
    if (_aftermath_dll)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${_aftermath_dll}
                $<TARGET_FILE_DIR:${TARGET_NAME}>
            VERBATIM)
    endif ()
endfunction()

set_target_properties(${HYLUX_RUNTIME_TARGET} PROPERTIES
    FOLDER "Hylux"
    DEBUG_POSTFIX "d"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source" FILES ${HYLUX_RUNTIME_SOURCES})
