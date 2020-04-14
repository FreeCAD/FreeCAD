
set(PACKAGE_WCREF "$WCREV$")
set(PACKAGE_WCDATE "$WCDATE$")
set(PACKAGE_WCURL "$WCURL$")

# If the sources don't include a Version.h then create one
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Version.h)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.h.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/Version.h.in)

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/Version.h
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/Version.h.out
            ${CMAKE_CURRENT_BINARY_DIR}/Version.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS version_check)

    add_custom_command(
        OUTPUT  version_check
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/SubWCRev.py
        --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    ADD_CUSTOM_TARGET(fc_version DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Version.h)

else (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Version.h)

    ADD_CUSTOM_TARGET(fc_version DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Version.h)

endif (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Version.h)
