# SPDX-FileNotice: Part of the FreeCAD project.

cmake_minimum_required(VERSION 3.22)

# Modern Swift QuickLook Extensions for macOS 15.0+
# This file handles building .appex extensions using Swift

# Configuration options (inherited from parent)
# CMAKE_OSX_DEPLOYMENT_TARGET

# Check for required tools
find_program(SWIFTC_EXECUTABLE swiftc REQUIRED)
find_program(PLUTIL_EXECUTABLE plutil REQUIRED)

# Find macOS SDK
execute_process(
    COMMAND xcrun --show-sdk-path --sdk macosx
    OUTPUT_VARIABLE MACOS_SDK_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE SDK_RESULT
)

if(NOT SDK_RESULT EQUAL 0 OR NOT EXISTS "${MACOS_SDK_PATH}")
    message(FATAL_ERROR "macOS SDK not found. Please install Xcode Command Line Tools.")
endif()

message(STATUS "Modern QuickLook: Using macOS SDK: ${MACOS_SDK_PATH}")

# Bundle identifiers (inherited from parent)
set(FREECAD_BUNDLE_ID "org.freecad.FreeCAD")
set(THUMBNAIL_BUNDLE_ID "${FREECAD_BUNDLE_ID}.quicklook.thumbnail")
set(PREVIEW_BUNDLE_ID "${FREECAD_BUNDLE_ID}.quicklook.preview")

# Paths (inherited from parent)
set(TARGET_APP_BUNDLE "${CMAKE_BINARY_DIR}/src/MacAppBundle/FreeCAD.app")
set(EXTENSIONS_DIR "${TARGET_APP_BUNDLE}/Contents/PlugIns")

# Build directories
set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(THUMBNAIL_EXT_DIR "${BUILD_DIR}/FreeCADThumbnailExtension.appex")
set(PREVIEW_EXT_DIR "${BUILD_DIR}/FreeCADPreviewExtension.appex")

# Detect target architecture
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    set(TARGET_ARCH "arm64")
else()
    set(TARGET_ARCH "x86_64")
endif()

# Swift compilation flags
set(SWIFT_FLAGS
    -target ${TARGET_ARCH}-apple-macosx${CMAKE_OSX_DEPLOYMENT_TARGET}
    -sdk ${MACOS_SDK_PATH}
    -O
    -whole-module-optimization
    -enable-library-evolution
    -swift-version 5
)



# Function to process Info.plist files
function(process_plist input_plist output_plist bundle_id executable_name product_name principal_class)
    add_custom_command(
        OUTPUT ${output_plist}
        COMMAND ${CMAKE_COMMAND} -E copy ${input_plist} ${output_plist}
        COMMAND ${PLUTIL_EXECUTABLE} -replace CFBundleIdentifier -string ${bundle_id} ${output_plist}
        COMMAND ${PLUTIL_EXECUTABLE} -replace CFBundleExecutable -string ${executable_name} ${output_plist}
        COMMAND ${PLUTIL_EXECUTABLE} -replace CFBundleName -string ${product_name} ${output_plist}
        COMMAND ${PLUTIL_EXECUTABLE} -replace NSExtension.NSExtensionPrincipalClass -string ${principal_class} ${output_plist}
        DEPENDS ${input_plist}
        COMMENT "Processing extension ${input_plist} -> ${output_plist}"
    )
endfunction()

# Create bundle directory structure
add_custom_target(create_modern_bundle_dirs ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTENSIONS_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${THUMBNAIL_EXT_DIR}/Contents/MacOS
    COMMAND ${CMAKE_COMMAND} -E make_directory ${THUMBNAIL_EXT_DIR}/Contents/Resources
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PREVIEW_EXT_DIR}/Contents/MacOS
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PREVIEW_EXT_DIR}/Contents/Resources
    COMMENT "Creating modern extensions directory structure"
)

# Process Info.plist files
process_plist(
    "${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailExtensionInfo.plist"
    "${THUMBNAIL_EXT_DIR}/Contents/Info.plist"
    ${THUMBNAIL_BUNDLE_ID}
    "FreeCADThumbnailExtension"
    "FreeCADThumbnailExtension"
    "FreeCADThumbnailExtension.ThumbnailProvider"
)

process_plist(
    "${CMAKE_CURRENT_SOURCE_DIR}/PreviewExtensionInfo.plist"
    "${PREVIEW_EXT_DIR}/Contents/Info.plist"
    ${PREVIEW_BUNDLE_ID}
    "FreeCADPreviewExtension"
    "FreeCADPreviewExtension"
    "FreeCADPreviewExtension.PreviewProvider"
)

# Compile Thumbnail Extension
add_custom_target(compile_thumbnail_extension ALL
    COMMAND ${SWIFTC_EXECUTABLE}
        ${SWIFT_FLAGS}
        -emit-executable
        -module-name FreeCADThumbnailExtension
        -parse-as-library
        -Xlinker -e -Xlinker _NSExtensionMain
        -Xlinker -rpath -Xlinker @executable_path/../Frameworks
        -Xlinker -rpath -Xlinker @executable_path/../../../../Frameworks
        -framework Foundation
        -framework CoreGraphics
        -framework ImageIO
        -framework QuickLookThumbnailing
        -framework AppKit
        -lcompression
        -o ${THUMBNAIL_EXT_DIR}/Contents/MacOS/FreeCADThumbnailExtension
        ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailProvider.swift
        ${CMAKE_CURRENT_SOURCE_DIR}/ZipExtractor.swift
    DEPENDS
        create_modern_bundle_dirs
        ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailProvider.swift
        ${CMAKE_CURRENT_SOURCE_DIR}/ZipExtractor.swift
        ${THUMBNAIL_EXT_DIR}/Contents/Info.plist
    COMMENT "Compiling Thumbnail Extension"
)

# Compile Preview Extension
add_custom_target(compile_preview_extension ALL
    COMMAND ${SWIFTC_EXECUTABLE}
        ${SWIFT_FLAGS}
        -emit-executable
        -module-name FreeCADPreviewExtension
        -parse-as-library
        -Xlinker -e -Xlinker _NSExtensionMain
        -Xlinker -rpath -Xlinker @executable_path/../Frameworks
        -Xlinker -rpath -Xlinker @executable_path/../../../../Frameworks
        -framework Foundation
        -framework CoreGraphics
        -framework ImageIO
        -framework Quartz
        -framework Cocoa
        -framework UniformTypeIdentifiers
        -lcompression
        -o ${PREVIEW_EXT_DIR}/Contents/MacOS/FreeCADPreviewExtension
        ${CMAKE_CURRENT_SOURCE_DIR}/PreviewProvider.swift
        ${CMAKE_CURRENT_SOURCE_DIR}/ZipExtractor.swift
    DEPENDS
        create_modern_bundle_dirs
        ${CMAKE_CURRENT_SOURCE_DIR}/PreviewProvider.swift
        ${CMAKE_CURRENT_SOURCE_DIR}/ZipExtractor.swift
        ${PREVIEW_EXT_DIR}/Contents/Info.plist
    COMMENT "Compiling Preview Extension"
)

# Embed unsigned extensions in main FreeCAD.app
add_custom_target(embed_modern_extensions ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${THUMBNAIL_EXT_DIR}
        ${EXTENSIONS_DIR}/FreeCADThumbnailExtension.appex
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PREVIEW_EXT_DIR}
        ${EXTENSIONS_DIR}/FreeCADPreviewExtension.appex
    DEPENDS
        compile_thumbnail_extension
        compile_preview_extension
    COMMENT "Embedding modern extensions in main FreeCAD.app"
)

# Main modern extensions target
add_custom_target(FreeCADModernQuickLook ALL
    DEPENDS embed_modern_extensions
    COMMENT "Modern Swift QuickLook extensions built"
)

# Install targets for modern extensions
install(DIRECTORY ${EXTENSIONS_DIR}/FreeCADThumbnailExtension.appex
    DESTINATION "${CMAKE_INSTALL_PREFIX}/PlugIns"
    COMPONENT ModernQuickLook
    USE_SOURCE_PERMISSIONS
)

install(DIRECTORY ${EXTENSIONS_DIR}/FreeCADPreviewExtension.appex
    DESTINATION "${CMAKE_INSTALL_PREFIX}/PlugIns"
    COMPONENT ModernQuickLook
    USE_SOURCE_PERMISSIONS
)

# Verification target
add_custom_target(verify_modern_build
    COMMAND test -f ${EXTENSIONS_DIR}/FreeCADThumbnailExtension.appex/Contents/MacOS/FreeCADThumbnailExtension
    COMMAND test -f ${EXTENSIONS_DIR}/FreeCADPreviewExtension.appex/Contents/MacOS/FreeCADPreviewExtension
    COMMAND echo "Modern extensions successfully embedded in FreeCAD.app"
    DEPENDS FreeCADModernQuickLook
    COMMENT "Verifying modern QuickLook extension integration"
)

# Test registration target
add_custom_target(test_modern_registration
    COMMAND pluginkit -a ${EXTENSIONS_DIR}/FreeCADThumbnailExtension.appex
    COMMAND pluginkit -a ${EXTENSIONS_DIR}/FreeCADPreviewExtension.appex
    COMMAND pluginkit -e use -i ${THUMBNAIL_BUNDLE_ID}
    COMMAND pluginkit -e use -i ${PREVIEW_BUNDLE_ID}
    COMMAND pluginkit -m -v -i ${THUMBNAIL_BUNDLE_ID} || echo "Thumbnail extension registration status unknown"
    COMMAND pluginkit -m -v -i ${PREVIEW_BUNDLE_ID} || echo "Preview extension registration status unknown"
    DEPENDS FreeCADModernQuickLook
    COMMENT "Testing modern QuickLook extension registration"
)

# Status messages
message(STATUS "Modern Swift QuickLook Extensions Configuration:")
message(STATUS "  Swift Compiler: ${SWIFTC_EXECUTABLE}")
message(STATUS "  Target Architecture: ${TARGET_ARCH}")
message(STATUS "  Deployment Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "  Thumbnail Bundle ID: ${THUMBNAIL_BUNDLE_ID}")
message(STATUS "  Preview Bundle ID: ${PREVIEW_BUNDLE_ID}")
message(STATUS "  Extensions Directory: ${EXTENSIONS_DIR}")
message(STATUS "  Code Signing: Handled by official macos_sign_and_notarize.sh script")
