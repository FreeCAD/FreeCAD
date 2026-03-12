# SPDX-FileNotice: Part of the FreeCAD project.

cmake_minimum_required(VERSION 3.22)

# Legacy Objective-C QuickLook Generator for older macOS compatibility
# This file handles building .qlgenerator bundles using Objective-C/C

# Configuration options (inherited from parent)
# CMAKE_OSX_DEPLOYMENT_TARGET

# Paths (inherited from parent)
set(TARGET_APP_BUNDLE "${CMAKE_BINARY_DIR}/src/MacAppBundle/FreeCAD.app")
set(LEGACY_QUICKLOOK_DIR "${TARGET_APP_BUNDLE}/Contents/Library/QuickLook")

# Build legacy QuickLook generator as a proper macOS plugin bundle
add_library(
    QuicklookFCStd
    MODULE
    GeneratePreviewForURL.m
    GenerateThumbnailForURL.m
    main.c
)

set_target_properties(
    QuicklookFCStd
    PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "qlgenerator"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/QuicklookFCStd.qlgenerator/Contents/Info.plist"
)

target_link_libraries(
    QuicklookFCStd
    "-framework AppKit"
    "-framework ApplicationServices"
    "-framework CoreData"
    "-framework CoreFoundation"
    "-framework CoreServices"
    "-framework Foundation"
    "-framework QuickLook"
)

set_target_properties(
    QuicklookFCStd
    PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks"
)

# Create bundle directory structure
add_custom_target(create_legacy_bundle_dirs ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LEGACY_QUICKLOOK_DIR}
    COMMENT "Creating legacy QuickLook directory structure"
)

# Install legacy generator to the system QuickLook directory within the app bundle
add_custom_target(embed_legacy_generator ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_BUNDLE_DIR:QuicklookFCStd>"
        "${LEGACY_QUICKLOOK_DIR}/QuicklookFCStd.qlgenerator"
    DEPENDS
        QuicklookFCStd
        create_legacy_bundle_dirs
    COMMENT "Embedding legacy QuickLook generator in main FreeCAD.app"
)

# Main legacy generator target
add_custom_target(FreeCADLegacyQuickLook ALL
    DEPENDS embed_legacy_generator
    COMMENT "Legacy Objective-C QuickLook generator built"
)

# Install target for legacy generator
install(DIRECTORY ${LEGACY_QUICKLOOK_DIR}/QuicklookFCStd.qlgenerator
    DESTINATION "${CMAKE_INSTALL_PREFIX}/Library/QuickLook"
    COMPONENT LegacyQuickLook
    USE_SOURCE_PERMISSIONS
)

# Verification target
add_custom_target(verify_legacy_build
    COMMAND test -f ${LEGACY_QUICKLOOK_DIR}/QuicklookFCStd.qlgenerator/Contents/MacOS/QuicklookFCStd
    COMMAND echo "Legacy generator successfully embedded in FreeCAD.app"
    DEPENDS FreeCADLegacyQuickLook
    COMMENT "Verifying legacy QuickLook generator integration"
)

# Test registration target
add_custom_target(test_legacy_registration
    COMMAND echo "Legacy QuickLook generator will be registered automatically by the system"
    COMMAND qlmanage -r || echo "QuickLook cache reset attempted"
    DEPENDS FreeCADLegacyQuickLook
    COMMENT "Testing legacy QuickLook generator registration"
)

# Status messages
message(STATUS "Legacy Objective-C QuickLook Generator Configuration:")
message(STATUS "  Target Library: QuicklookFCStd")
message(STATUS "  Deployment Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "  Legacy QuickLook Directory: ${LEGACY_QUICKLOOK_DIR}")
message(STATUS "  Bundle ID: org.freecad.qlgenerator.QuicklookFCStd")
message(STATUS "  Code Signing: Handled by official macos_sign_and_notarize.sh script")
