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
        add_custom_command(TARGET ${HYLUX_RUNTIME_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Nvidia::Aftermath>
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source"
                     FILES ${HYLUX_CRASH_AFTERMATH_SOURCES})
    else ()
        message(STATUS "Hylux: HYLUX_CRASH_AFTERMATH active but Nvidia::Aftermath "
                       "target not available; crash reporter will return null at runtime.")
    endif ()
endif ()

set_target_properties(${HYLUX_RUNTIME_TARGET} PROPERTIES
    FOLDER "Hylux"
    DEBUG_POSTFIX "d"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source" FILES ${HYLUX_RUNTIME_SOURCES})
