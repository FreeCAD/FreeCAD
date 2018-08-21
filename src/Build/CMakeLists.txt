
set(PACKAGE_WCREF "$WCREV$")
set(PACKAGE_WCDATE "$WCDATE$")
set(PACKAGE_WCURL "$WCURL$")

# If the sources don't include a Version.h then create one
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Version.h)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/Version.h.in
)

EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/Tools/SubWCRev.py
      --srcdir=${CMAKE_SOURCE_DIR} --bindir=${CMAKE_BINARY_DIR}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_STRIP_TRAILING_WHITESPACE)

fc_copy_file_if_different(
    "${CMAKE_CURRENT_BINARY_DIR}/Version.h.out"
    "${CMAKE_CURRENT_BINARY_DIR}/Version.h"
)

# as fallback if Version.h isn't created for any reason
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/Version.h)
    set(PACKAGE_WCREF ${PACKAGE_VERSION_PATCH})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.h.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/Version.h
    )
endif (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/Version.h)
endif (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Version.h)
