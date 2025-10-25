cmake_minimum_required(VERSION 3.22)
project(FreeCADQuickLookSwift)

# Enable Swift language support
enable_language(Swift)

# Set deployment target for modern QuickLook features - support macOS 15+
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")

# Swift compiler flags - Objective-C interop is enabled by default in modern Swift
# Explicitly disable deprecated flags for Swift 6.2+
set(CMAKE_Swift_FLAGS "")
set(CMAKE_Swift_FLAGS_DEBUG "-Onone -g")
set(CMAKE_Swift_FLAGS_RELEASE "-O")

# Define the QuickLook Thumbnail Extension target
add_executable(FreeCADThumbnailExtension
    ThumbnailProvider.swift
)

# Disable Qt automoc for Swift targets
set_target_properties(FreeCADThumbnailExtension PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

# Define the QuickLook Preview Extension target
add_executable(FreeCADPreviewExtension
    PreviewProvider.swift
)

# Disable Qt automoc for Swift targets
set_target_properties(FreeCADPreviewExtension PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

# Set App Extension properties
set_target_properties(FreeCADThumbnailExtension
    PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "appex"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    MACOSX_DEPLOYMENT_TARGET "15.0"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_EXECUTABLE_NAME "FreeCADThumbnailExtension"
    # Swift-specific properties
    Swift_MODULE_NAME "FreeCADThumbnailExtension"
    Swift_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/swift_modules"
    # App Extension specific settings
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY "YES"
    XCODE_ATTRIBUTE_SKIP_INSTALL "YES"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
    XCODE_ATTRIBUTE_PRODUCT_TYPE "com.apple.product-type.app-extension"
    XCODE_ATTRIBUTE_MACH_O_TYPE "mh_bundle"
    # Performance optimizations
    XCODE_ATTRIBUTE_SWIFT_OPTIMIZATION_LEVEL "-O"
    # Disable deprecated Swift flags
    XCODE_ATTRIBUTE_ENABLE_OBJC_INTEROP "NO"
)

# Link required frameworks for Swift QuickLook extension
target_link_libraries(FreeCADThumbnailExtension
    "-framework Foundation"
    "-framework CoreGraphics"
    "-framework ImageIO"
    "-framework QuickLookThumbnailing"
    "-framework AppKit"
    "-lcompression"
    "-framework OSLog"
)

# Set Swift language version
set_property(TARGET FreeCADThumbnailExtension PROPERTY Swift_LANGUAGE_VERSION 5.0)

# Set App Extension properties for Preview Extension
set_target_properties(FreeCADPreviewExtension PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "appex"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/PreviewInfo.plist"
    MACOSX_DEPLOYMENT_TARGET "15.0"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_EXECUTABLE_NAME "FreeCADPreviewExtension"
    # App Extension specific settings
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY "YES"
    XCODE_ATTRIBUTE_SKIP_INSTALL "YES"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
    XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
    XCODE_ATTRIBUTE_PRODUCT_TYPE "com.apple.product-type.app-extension"
    XCODE_ATTRIBUTE_MACH_O_TYPE "mh_bundle"
    # Performance optimizations
    XCODE_ATTRIBUTE_SWIFT_OPTIMIZATION_LEVEL "-O"
    # Disable deprecated Swift flags
    XCODE_ATTRIBUTE_ENABLE_OBJC_INTEROP "NO"
    Swift_MODULE_NAME "FreeCADPreviewExtension"
    Swift_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/swift_modules_preview"
)

# Link required frameworks for Swift Preview extension
target_link_libraries(FreeCADPreviewExtension
    "-framework Foundation"
    "-framework CoreGraphics"
    "-framework ImageIO"
    "-framework QuickLookUI"
    "-framework AppKit"
    "-lcompression"
    "-framework OSLog"
)

# Set Swift language version for Preview Extension
set_property(TARGET FreeCADPreviewExtension PROPERTY Swift_LANGUAGE_VERSION 5.0)

# Host app target (minimal container for FreeCAD integration)
add_executable(FreeCADQuickLookHost
    HostApp.swift
)

# Disable Qt automoc for Swift targets
set_target_properties(FreeCADQuickLookHost PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

set_target_properties(FreeCADQuickLookHost
    PROPERTIES
    BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/FreeCADHostInfo.plist"
    MACOSX_DEPLOYMENT_TARGET "15.0"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_EXECUTABLE_NAME "FreeCADQuickLookHost"
    Swift_MODULE_NAME "FreeCADQuickLookHost"
    Swift_MODULE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/swift_modules_host"
    # Fix bundle identifier mismatch
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.freecad.quicklook.host"
    # Disable deprecated Swift flags
    XCODE_ATTRIBUTE_ENABLE_OBJC_INTEROP "NO"
)

target_link_libraries(FreeCADQuickLookHost
    "-framework Foundation"
    "-framework AppKit"
)

# Embed the extension in the host app
# Embed extensions in host app
add_custom_command(TARGET FreeCADQuickLookHost POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_DIR:FreeCADQuickLookHost>/Contents/PlugIns"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_BUNDLE_DIR:FreeCADThumbnailExtension>"
        "$<TARGET_BUNDLE_DIR:FreeCADQuickLookHost>/Contents/PlugIns/FreeCADThumbnailExtension.appex"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_BUNDLE_DIR:FreeCADPreviewExtension>"
        "$<TARGET_BUNDLE_DIR:FreeCADQuickLookHost>/Contents/PlugIns/FreeCADPreviewExtension.appex"
    COMMENT "Embedding QuickLook thumbnail and preview extensions in host app"
)

# Ensure the extensions are built before the host app
add_dependencies(FreeCADQuickLookHost FreeCADThumbnailExtension FreeCADPreviewExtension)

# FreeCAD integration installation rules (Option 1: Embed in FreeCAD.app)
if(FREECAD_CREATE_MAC_APP OR (APPLE AND BUILD_WITH_CONDA))
    # Install QuickLook host app directly into FreeCAD.app bundle
    install(TARGETS FreeCADQuickLookHost
        BUNDLE DESTINATION "${CMAKE_INSTALL_PREFIX}/Contents/Library/QuickLook"
        COMPONENT QuickLook
    )

    # Install a registration script for post-install QuickLook registration
    install(CODE "
        # Register QuickLook extensions with the system
        set(QUICKLOOK_HOST_PATH \"${CMAKE_INSTALL_PREFIX}/Contents/Library/QuickLook/FreeCADQuickLookHost.app\")

        # Create a registration script that FreeCAD can call on first launch
        file(WRITE \"${CMAKE_INSTALL_PREFIX}/Contents/Resources/register_quicklook.sh\"
            \"#!/bin/bash\\n\"
            \"# FreeCAD QuickLook Registration Script\\n\"
            \"QUICKLOOK_HOST=\\\"${CMAKE_INSTALL_PREFIX}/Contents/Library/QuickLook/FreeCADQuickLookHost.app\\\"\\n\"
            \"if [ -d \\\"\\$QUICKLOOK_HOST\\\" ]; then\\n\"
            \"    echo \\\"Registering FreeCAD QuickLook extensions...\\\"\\n\"
            \"    pluginkit -a \\\"\\$QUICKLOOK_HOST\\\" 2>/dev/null || true\\n\"
            \"    qlmanage -r 2>/dev/null || true\\n\"
            \"    echo \\\"FreeCAD QuickLook extensions registered.\\\"\\n\"
            \"else\\n\"
            \"    echo \\\"QuickLook host app not found at \\$QUICKLOOK_HOST\\\"\\n\"
            \"fi\\n\"
        )

        # Make the script executable
        execute_process(COMMAND chmod +x \"${CMAKE_INSTALL_PREFIX}/Contents/Resources/register_quicklook.sh\")

        message(STATUS \"FreeCAD QuickLook extensions installed to: \\${CMAKE_INSTALL_PREFIX}/Contents/Library/QuickLook/FreeCADQuickLookHost.app\")
        message(STATUS \"Registration script created at: \\${CMAKE_INSTALL_PREFIX}/Contents/Resources/register_quicklook.sh\")
    " COMPONENT QuickLook)
else()
    # Standalone installation for development/testing
    install(TARGETS FreeCADQuickLookHost
        BUNDLE DESTINATION "/Applications"
        COMPONENT QuickLook
    )

    # For development: immediate registration
    install(CODE "
        execute_process(
            COMMAND pluginkit -a \"/Applications/FreeCADQuickLookHost.app\"
            ERROR_QUIET
        )
        execute_process(
            COMMAND qlmanage -r
            ERROR_QUIET
        )
        message(STATUS \"Development QuickLook extensions registered\")
    " COMPONENT QuickLook)
endif()

# Development and testing targets
add_custom_target(install_quicklook_dev
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install --config $<CONFIG>
    COMMAND pluginkit -a "/Applications/FreeCADQuickLookHost.app" || true
    COMMAND qlmanage -r
    COMMENT "Installing Swift QuickLook extensions for development and reloading QuickLook"
    DEPENDS FreeCADQuickLookHost
)

add_custom_target(uninstall_quicklook_dev
    COMMAND pluginkit -r "/Applications/FreeCADQuickLookHost.app" || true
    COMMAND rm -rf "/Applications/FreeCADQuickLookHost.app"
    COMMAND qlmanage -r
    COMMENT "Uninstalling Swift QuickLook extensions and reloading QuickLook"
)

# FreeCAD-specific targets
add_custom_target(install_quicklook_freecad
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install --config $<CONFIG>
    COMMENT "Installing Swift QuickLook extensions into FreeCAD.app bundle"
    DEPENDS FreeCADQuickLookHost
)

add_custom_target(register_quicklook_freecad
    COMMAND pluginkit -a "${CMAKE_INSTALL_PREFIX}/Contents/Library/QuickLook/FreeCADQuickLookHost.app" || true
    COMMAND qlmanage -r
    COMMENT "Registering FreeCAD QuickLook extensions with the system"
)

# Test target for validating the Swift implementation
add_custom_target(test_swift_zip_parser
    COMMAND swift "${CMAKE_CURRENT_SOURCE_DIR}/ValidateFixedParser.swift" "${CMAKE_CURRENT_SOURCE_DIR}/../base.FCStd"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Testing Swift ZIP parser implementation"
)

# Print build information
message(STATUS "FreeCAD Swift QuickLook Configuration:")
message(STATUS "  Integration Mode: Embedded in FreeCAD.app (Option 1)")
message(STATUS "  Swift Version: ${CMAKE_Swift_COMPILER_VERSION}")
message(STATUS "  Target: ${CMAKE_SYSTEM_NAME} ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Swift Flags: ${CMAKE_Swift_FLAGS}")
message(STATUS "  Extensions: Thumbnail + Preview (.appex)")
message(STATUS "  Pure Swift: No external dependencies")
message(STATUS "  Install Path: \${CMAKE_INSTALL_PREFIX}/Contents/Library/QuickLook/")
message(STATUS "  Bundle ID: org.freecad.quicklook.host")

# Verify Swift compiler is available
if(NOT CMAKE_Swift_COMPILER)
    message(FATAL_ERROR "Swift compiler not found. Please install Xcode with Swift support.")
endif()

# Verify minimum deployment target
if(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS "15.0")
    message(WARNING "Swift QuickLook extensions require macOS 15.0+. Current target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()
