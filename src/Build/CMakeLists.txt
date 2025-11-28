set(PACKAGE_WCREF "$WCREV$")		# replace these names with others
set(PACKAGE_WCDATE "$WCDATE$")		# for next replacement in SubWCRev.py
set(PACKAGE_WCURL "$WCURL$")		# for final version.h and freecad.rc 

# create .in file with version info, still missing data from RCS
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.h.template
            ${CMAKE_CURRENT_BINARY_DIR}/Version.h.in)

# add data from RCS to version and rc files
add_custom_command(
    OUTPUT version_file
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/Version.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/SubWCRev.py
        --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/Version.h.out
        ${CMAKE_CURRENT_BINARY_DIR}/Version.h
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/src/Main/freecad.rc.out
        ${CMAKE_BINARY_DIR}/src/Main/freecad.rc
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/src/Main/freecadCMD.rc.out
        ${CMAKE_BINARY_DIR}/src/Main/freecadCMD.rc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Version.h.in)

set_source_files_properties(Version.h PROPERTIES SYMBOLIC true)

add_custom_target(fc_version DEPENDS version_file)
