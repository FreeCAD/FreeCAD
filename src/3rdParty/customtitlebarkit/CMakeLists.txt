cmake_minimum_required(VERSION 3.21)
project(customtitlebarkit VERSION 0.1.0 LANGUAGES CXX)

# Detect standalone vs subdirectory build
set(CUSTOMTITLEBARKIT_STANDALONE OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CUSTOMTITLEBARKIT_STANDALONE ON)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_AUTOMOC ON)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

include(GenerateExportHeader)

add_library(customtitlebarkit SHARED
    include/customtitlebarkit/CustomTitleBarWindow.h
    include/customtitlebarkit/TitleBarWidget.h
    include/customtitlebarkit/FoldableMenuBar.h
    src/CustomTitleBarWindow.cpp
    src/TitleBarWidget.cpp
    src/FoldableMenuBar.cpp
    include/customtitlebarkit/MenuIntegration.h
    src/MenuIntegration.cpp
    src/WindowDecorationButton.h
    src/WindowDecorationButton.cpp
    src/platform/PlatformTitleBarBackend.h
    src/platform/PlatformTitleBarBackend.cpp
    src/platform/generic/GenericTitleBarBackend.h
    src/platform/generic/GenericTitleBarBackend.cpp
)

generate_export_header(customtitlebarkit
    EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/include/customtitlebarkit/export.h
    EXPORT_MACRO_NAME CUSTOMTITLEBARKIT_EXPORT
)

target_include_directories(customtitlebarkit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(customtitlebarkit PROPERTIES AUTOMOC ON)

target_link_libraries(customtitlebarkit PUBLIC Qt6::Widgets)

if(APPLE)
    enable_language(OBJCXX)

    # MacGenericBackend is always available on macOS (frameless + traffic lights)
    target_sources(customtitlebarkit PRIVATE
        src/platform/mac/MacGenericBackend.h
        src/platform/mac/MacGenericBackend.mm
    )
    set_source_files_properties(src/platform/mac/MacGenericBackend.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
    target_link_libraries(customtitlebarkit PRIVATE "-framework AppKit")

    if(Qt6_VERSION VERSION_GREATER_EQUAL "6.10")
        # Native titlebar backend using Qt 6.10 APIs
        target_sources(customtitlebarkit PRIVATE
            src/platform/mac/MacTitleBarBackend.h
            src/platform/mac/MacTitleBarBackend.mm
        )
        set_source_files_properties(src/platform/mac/MacTitleBarBackend.mm
            PROPERTIES COMPILE_FLAGS "-fobjc-arc"
        )
    endif()
elseif(WIN32)
    target_sources(customtitlebarkit PRIVATE
        src/platform/win/WinTitleBarBackend.cpp
    )
    target_link_libraries(customtitlebarkit PRIVATE dwmapi)
elseif(UNIX)
    find_package(Qt6 COMPONENTS Svg)
    target_sources(customtitlebarkit PRIVATE
        src/platform/linux/LinuxTitleBarBackend.h
        src/platform/linux/LinuxTitleBarBackend.cpp
        src/platform/linux/LinuxWindowButton.h
        src/platform/linux/LinuxWindowButton.cpp
        src/platform/linux/LinuxDesktopDetection.h
        src/platform/linux/LinuxDesktopDetection.cpp
    )
    if(Qt6Svg_FOUND)
        target_link_libraries(customtitlebarkit PRIVATE Qt6::Svg)
    endif()

    find_package(Qt6 COMPONENTS GuiPrivate)
    if(Qt6GuiPrivate_FOUND)
        target_link_libraries(customtitlebarkit PRIVATE Qt6::GuiPrivate)
        target_compile_definitions(customtitlebarkit PRIVATE HAVE_QT_GUI_PRIVATE=1)
    endif()
endif()

if(CUSTOMTITLEBARKIT_STANDALONE)
    add_subdirectory(examples/basic)
endif()
