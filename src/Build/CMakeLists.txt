set(PACKAGE_WCREF "$WCREV$")
set(PACKAGE_WCDATE "$WCDATE$")
set(PACKAGE_WCURL "$WCURL$")

# Source archives carry revision metadata for builds without Git.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/FreeCADVersion.txt")
    configure_file(FreeCADVersion.txt FreeCADVersion.txt COPYONLY)
    add_custom_target(fc_version)
else()
    configure_file(FreeCADVersion.txt.cmake FreeCADVersion.txt.in)
    add_custom_target(fc_version
        BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/FreeCADVersion.txt"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/src/Tools/SubWCRev.py"
            "--srcdir=${CMAKE_SOURCE_DIR}" "--bindir=${CMAKE_BINARY_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_BINARY_DIR}/FreeCADVersion.txt.out"
            "${CMAKE_CURRENT_BINARY_DIR}/FreeCADVersion.txt"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        VERBATIM
    )
endif()

# Keep metadata beside the library in both build and installed trees.
add_custom_command(TARGET fc_version POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:FreeCADBase>"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/FreeCADVersion.txt"
        "$<TARGET_FILE_DIR:FreeCADBase>/FreeCADVersion.txt"
    VERBATIM
)

if(WIN32)
    set(version_install_dir "${CMAKE_INSTALL_BINDIR}")
else()
    set(version_install_dir "${CMAKE_INSTALL_LIBDIR}")
endif()
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/FreeCADVersion.txt"
    DESTINATION "${version_install_dir}")
