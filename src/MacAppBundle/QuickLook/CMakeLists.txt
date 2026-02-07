# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

cmake_minimum_required(VERSION 3.22)
project(FreeCADQuickLook)

# Configuration options for QuickLook support
option(FREECAD_QUICKLOOK_LEGACY_SUPPORT "Build legacy .qlgenerator for older macOS compatibility" ON)
option(FREECAD_QUICKLOOK_MODERN_SUPPORT "Build modern .appex extensions for macOS 15.0+" ON)


# Only build QuickLook extensions with Unix Makefiles or Ninja generator
if(NOT CMAKE_GENERATOR MATCHES "Unix Makefiles|Ninja")
    message(STATUS "QuickLook extensions only supported with Unix Makefiles or Ninja generators (current: ${CMAKE_GENERATOR})")
    add_custom_target(FreeCADQuickLook
        COMMAND ${CMAKE_COMMAND} -E echo "QuickLook extensions require Unix Makefiles or Ninja generator"
        COMMENT "QuickLook placeholder target"
    )
    return()
endif()

# Validate CMAKE_OSX_DEPLOYMENT_TARGET is set (or use system default)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    # If not explicitly set, query the system default
    execute_process(
        COMMAND xcrun --show-sdk-version
        OUTPUT_VARIABLE MACOS_SDK_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(MACOS_SDK_VERSION)
        set(CMAKE_OSX_DEPLOYMENT_TARGET ${MACOS_SDK_VERSION})
        message(STATUS "CMAKE_OSX_DEPLOYMENT_TARGET not set, using SDK version: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    else()
        message(WARNING "CMAKE_OSX_DEPLOYMENT_TARGET not set and could not determine SDK version. Defaulting to 15.0")
        set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
    endif()
endif()

# Determine which QuickLook implementations to build
set(BUILD_MODERN_EXTENSIONS OFF)
set(BUILD_LEGACY_GENERATOR OFF)

if(FREECAD_QUICKLOOK_MODERN_SUPPORT)
    if(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_GREATER_EQUAL "15.0")
        set(BUILD_MODERN_EXTENSIONS ON)
        message(STATUS "Building modern Swift .appex QuickLook extensions (macOS 15.0+)")
    else()
        message(STATUS "Modern QuickLook extensions require macOS 15.0+, current target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()
endif()

if(FREECAD_QUICKLOOK_LEGACY_SUPPORT)
    if(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "15.0")
        set(BUILD_LEGACY_GENERATOR ON)
        message(STATUS "Building legacy .qlgenerator QuickLook support")
    else()
        message(STATUS "Skipping legacy .qlgenerator (not needed for macOS 15.0+)")
    endif()
endif()

if(NOT BUILD_MODERN_EXTENSIONS AND NOT BUILD_LEGACY_GENERATOR)
    message(WARNING "No QuickLook implementations enabled. Enable FREECAD_QUICKLOOK_MODERN_SUPPORT or FREECAD_QUICKLOOK_LEGACY_SUPPORT")
    add_custom_target(FreeCADQuickLook
        COMMAND ${CMAKE_COMMAND} -E echo "No QuickLook implementations enabled"
        COMMENT "QuickLook disabled target"
    )
    return()
endif()

# Global configuration
set(FREECAD_BUNDLE_ID "org.freecad.FreeCAD")
set(TARGET_APP_BUNDLE "${CMAKE_BINARY_DIR}/src/MacAppBundle/FreeCAD.app")

# Detect target architecture
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    set(TARGET_ARCH "arm64")
else()
    set(TARGET_ARCH "x86_64")
endif()

# Build subdirectories conditionally
set(QUICKLOOK_TARGETS "")

if(BUILD_MODERN_EXTENSIONS)
    add_subdirectory(modern)
    list(APPEND QUICKLOOK_TARGETS FreeCADModernQuickLook)
endif()

if(BUILD_LEGACY_GENERATOR)
    add_subdirectory(legacy)
    list(APPEND QUICKLOOK_TARGETS FreeCADLegacyQuickLook)
endif()

# Main target that coordinates all QuickLook implementations
add_custom_target(FreeCADQuickLook ALL
    DEPENDS ${QUICKLOOK_TARGETS}
    COMMENT "FreeCAD QuickLook support integrated into main app"
)

# Ensure QuickLook is built after main FreeCAD app if it's available as a target
if(TARGET FreeCAD)
    add_dependencies(FreeCADQuickLook FreeCAD)
endif()

# Combined verification target
set(VERIFY_COMMANDS "")
if(BUILD_MODERN_EXTENSIONS)
    list(APPEND VERIFY_COMMANDS
        COMMAND ${CMAKE_COMMAND} --build . --target verify_modern_build
    )
endif()
if(BUILD_LEGACY_GENERATOR)
    list(APPEND VERIFY_COMMANDS
        COMMAND ${CMAKE_COMMAND} --build . --target verify_legacy_build
    )
endif()

add_custom_target(verify_build
    ${VERIFY_COMMANDS}
    DEPENDS FreeCADQuickLook
    COMMENT "Verifying all QuickLook implementations"
)

# Combined test registration target
set(REGISTRATION_COMMANDS "")
if(BUILD_MODERN_EXTENSIONS)
    list(APPEND REGISTRATION_COMMANDS
        COMMAND ${CMAKE_COMMAND} --build . --target test_modern_registration
    )
endif()
if(BUILD_LEGACY_GENERATOR)
    list(APPEND REGISTRATION_COMMANDS
        COMMAND ${CMAKE_COMMAND} --build . --target test_legacy_registration
    )
endif()

add_custom_target(test_registration
    ${REGISTRATION_COMMANDS}
    DEPENDS FreeCADQuickLook
    COMMENT "Testing all QuickLook registrations"
)

# Installation components
if(BUILD_MODERN_EXTENSIONS)
    # Modern extensions install themselves via their CMakeLists.txt
endif()

if(BUILD_LEGACY_GENERATOR)
    # Legacy generator installs itself via its CMakeLists.txt
endif()

# Status summary
message(STATUS "FreeCAD QuickLook Configuration Summary:")
message(STATUS "  Generator: ${CMAKE_GENERATOR}")
message(STATUS "  Target Architecture: ${TARGET_ARCH}")
message(STATUS "  Deployment Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "  Target FreeCAD.app: ${TARGET_APP_BUNDLE}")
message(STATUS "  Modern Extensions (.appex): ${BUILD_MODERN_EXTENSIONS}")
message(STATUS "  Legacy Generator (.qlgenerator): ${BUILD_LEGACY_GENERATOR}")

message(STATUS "  Code Signing: Handled by official src/Tools/macos_sign_and_notarize.sh script")
