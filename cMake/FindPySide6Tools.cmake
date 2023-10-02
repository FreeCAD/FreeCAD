# Try to find PYSIDE6 utilities, PYSIDE6UIC and PYSIDE6RCC:
# PYSIDE_UIC_EXECUTABLE - Location of PYSIDE6UIC executable
# PYSIDE_RCC_EXECUTABLE - Location of PYSIDE6RCC executable
# PYSIDE_TOOLS_FOUND - PYSIDE6 utilities found.

if (TARGET Qt6::uic)
    get_target_property(PYSIDE6_UIC_EXECUTABLE Qt6::uic LOCATION)
    set(UICOPTIONS "--generator=python")
endif()
if (TARGET Qt6::rcc)
    get_target_property(PYSIDE6_RCC_EXECUTABLE Qt6::rcc LOCATION)
    set(RCCOPTIONS "--generator=python" "--compress-algo=zlib" "--compress=1")
endif()

set(PYSIDE_RCC_EXECUTABLE ${PYSIDE6_RCC_EXECUTABLE})
set(PYSIDE_UIC_EXECUTABLE ${PYSIDE6_UIC_EXECUTABLE})
set(PySideTools_VERSION 6)

if(PYSIDE_RCC_EXECUTABLE AND PYSIDE_UIC_EXECUTABLE)
    set(PYSIDE_TOOLS_FOUND TRUE)
    if (NOT PYSIDE6Tools_FIND_QUIETLY)
        message(STATUS "Found PYSIDE6 tools: ${PYSIDE_UIC_EXECUTABLE}, ${PYSIDE_RCC_EXECUTABLE}")
    endif ()
else()
    if(PYSIDE6Tools_FIND_REQUIRED)
        message(FATAL_ERROR "PYSIDE6 tools could not be found, but are required.")
    else()
        if (NOT PYSIDE6Tools_FIND_QUIETLY)
            message(STATUS "PYSIDE6 tools: not found.")
        endif ()
    endif()
endif()
