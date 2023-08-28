# Try to find PySide2 utilities, PYSIDE2UIC and PYSIDE2RCC:
# PYSIDE_UIC_EXECUTABLE - Location of PYSIDE2UIC executable
# PYSIDE_RCC_EXECUTABLE - Location of PYSIDE2RCC executable
# PYSIDE_TOOLS_FOUND - PySide2 utilities found.

if(WIN32 OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    #pyside2 tools are often in same location as python interpreter
    get_filename_component(PYTHON_BIN_DIR ${PYTHON_EXECUTABLE} PATH)
    set(PYSIDE_BIN_DIR ${PYTHON_BIN_DIR})
endif(WIN32 OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Since Qt v5.14, pyside2-uic and pyside2-rcc are directly provided by Qt5Core uic and rcc, with '-g python' option
# We test Qt5Core version to act accordingly

FIND_PACKAGE(Qt5 COMPONENTS Core Widgets)

IF(Qt5Core_VERSION VERSION_LESS 5.14)
  # Legacy (< 5.14)
  FIND_PROGRAM(PYSIDE2_UIC_EXECUTABLE NAMES python2-pyside2-uic pyside2-uic pyside2-uic-${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} pyuic5 HINTS ${PYSIDE_BIN_DIR})
  FIND_PROGRAM(PYSIDE2_RCC_EXECUTABLE NAMES pyside2-rcc pyside2-rcc-${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} pyrcc5 HINTS ${PYSIDE_BIN_DIR})
  set(UICOPTIONS "")
  set(RCCOPTIONS "")
ELSE()
  # New (>= 5.14)
  if (TARGET Qt::uic)
    get_property(PYSIDE2_UIC_EXECUTABLE TARGET Qt::uic PROPERTY LOCATION)
    set(UICOPTIONS "--generator=python")
  endif()
  if (TARGET Qt::rcc)
    get_property(PYSIDE2_RCC_EXECUTABLE TARGET Qt::rcc PROPERTY LOCATION)
    set(RCCOPTIONS "--generator=python" "--compress-algo=zlib" "--compress=1")
  endif()
ENDIF()

set(PYSIDE_RCC_EXECUTABLE ${PYSIDE2_RCC_EXECUTABLE})
set(PYSIDE_UIC_EXECUTABLE ${PYSIDE2_UIC_EXECUTABLE})
set(PySideTools_VERSION 2)

if(PYSIDE_RCC_EXECUTABLE AND PYSIDE_UIC_EXECUTABLE)
    set(PYSIDE_TOOLS_FOUND TRUE)
    if (NOT PySide2Tools_FIND_QUIETLY)
        message(STATUS "Found PySide2 tools: ${PYSIDE_UIC_EXECUTABLE}, ${PYSIDE_RCC_EXECUTABLE}")
    endif (NOT PySide2Tools_FIND_QUIETLY)
else()
    if(PySide2Tools_FIND_REQUIRED)
        message(FATAL_ERROR "PySide2 tools could not be found, but are required.")
    else(PySide2Tools_FIND_REQUIRED)
        if (NOT PySide2Tools_FIND_QUIETLY)
                message(STATUS "PySide2 tools: not found.")
        endif (NOT PySide2Tools_FIND_QUIETLY)
    endif(PySide2Tools_FIND_REQUIRED)
endif()
