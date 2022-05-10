# Try to find PySide2 utilities, PYSIDE2UIC and PYSIDE2RCC:
# PYSIDE2UICBINARY - Location of PYSIDE2UIC executable
# PYSIDE2RCCBINARY - Location of PYSIDE2RCC executable
# PYSIDE2_TOOLS_FOUND - PySide2 utilities found.

# Also provides macro similar to FindQt4.cmake's WRAP_UI and WRAP_RC,
# for the automatic generation of Python code from Qt4's user interface
# ('.ui') and resource ('.qrc') files. These macros are called:
# - PYSIDE_WRAP_UI
# - PYSIDE_WRAP_RC

IF(PYSIDE2UICBINARY AND PYSIDE2RCCBINARY)
  # Already in cache, be silent
  set(PYSIDE2_TOOLS_FOUND_QUIETLY TRUE)
ENDIF(PYSIDE2UICBINARY AND PYSIDE2RCCBINARY)

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
  FIND_PROGRAM(PYSIDE2UICBINARY NAMES python2-pyside2-uic pyside2-uic pyside2-uic-${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} pyuic5 HINTS ${PYSIDE_BIN_DIR})
  FIND_PROGRAM(PYSIDE2RCCBINARY NAMES pyside2-rcc pyside2-rcc-${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} pyrcc5 HINTS ${PYSIDE_BIN_DIR})
  set(UICOPTIONS "")
  set(RCCOPTIONS "")
ELSE()
  # New (>= 5.14)
  if (TARGET Qt::uic)
    get_property(PYSIDE2UICBINARY TARGET Qt::uic PROPERTY LOCATION)
    set(UICOPTIONS "--generator=python")
  endif()
  if (TARGET Qt::rcc)
    get_property(PYSIDE2RCCBINARY TARGET Qt::rcc PROPERTY LOCATION)
    set(RCCOPTIONS "--generator=python" "--compress-algo=zlib" "--compress=1")
  endif()
ENDIF(Qt5Core_VERSION VERSION_LESS 5.14)

MACRO(PYSIDE_WRAP_UI outfiles)
  if (NOT PYSIDE2UICBINARY)
    message(FATAL_ERROR "Qt uic is required for generating ${ARGN}")
  endif()
  FOREACH(it ${ARGN})
    GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.py)
    #ADD_CUSTOM_TARGET(${it} ALL
    #  DEPENDS ${outfile}
    #)
    if(WIN32 OR APPLE)
        ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
          COMMAND ${PYSIDE2UICBINARY} ${UICOPTIONS} ${infile} -o ${outfile}
          MAIN_DEPENDENCY ${infile}
        )
    else()
        # Especially on Open Build Service we don't want changing date like
        # pyside2-uic generates in comments at beginning., which is why
        # we follow the tool command with a POSIX-friendly sed.
        ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
          COMMAND "${PYSIDE2UICBINARY}" ${UICOPTIONS} "${infile}" -o "${outfile}"
          COMMAND sed "/^# /d" "${outfile}" >"${outfile}.tmp" && mv "${outfile}.tmp" "${outfile}"
          MAIN_DEPENDENCY "${infile}"
        )
    endif()
    list(APPEND ${outfiles} ${outfile})
  ENDFOREACH(it)
ENDMACRO (PYSIDE_WRAP_UI)

MACRO(PYSIDE_WRAP_RC outfiles)
  if (NOT PYSIDE2RCCBINARY)
    message(FATAL_ERROR "Qt rcc is required for generating ${ARGN}")
  endif()
  FOREACH(it ${ARGN})
    GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    SET(outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfile}_rc.py")
    #ADD_CUSTOM_TARGET(${it} ALL
    #  DEPENDS ${outfile}
    #)
    if(WIN32 OR APPLE)
        ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
          COMMAND ${PYSIDE2RCCBINARY} ${RCCOPTIONS} ${infile} -o ${outfile}
          MAIN_DEPENDENCY ${infile}
        )
    else()
        # Especially on Open Build Service we don't want changing date like
        # pyside-rcc generates in comments at beginning, which is why
        # we follow the tool command with in-place sed.
        ADD_CUSTOM_COMMAND(OUTPUT "${outfile}"
          COMMAND "${PYSIDE2RCCBINARY}" ${RCCOPTIONS} "${infile}" ${PY_ATTRIBUTE} -o "${outfile}"
          COMMAND sed "/^# /d" "${outfile}" >"${outfile}.tmp" && mv "${outfile}.tmp" "${outfile}"
          MAIN_DEPENDENCY "${infile}"
        )
    endif()
    list(APPEND ${outfiles} ${outfile})
  ENDFOREACH(it)
ENDMACRO (PYSIDE_WRAP_RC)

if(PYSIDE2RCCBINARY AND PYSIDE2UICBINARY)
    set(PYSIDE2_TOOLS_FOUND TRUE)
    if (NOT PySide2Tools_FIND_QUIETLY)
        message(STATUS "Found PySide2 tools: ${PYSIDE2UICBINARY}, ${PYSIDE2RCCBINARY}")
    endif (NOT PySide2Tools_FIND_QUIETLY)
else(PYSIDE2RCCBINARY AND PYSIDE2UICBINARY)
    if(PySide2Tools_FIND_REQUIRED)
        message(FATAL_ERROR "PySide2 tools could not be found, but are required.")
    else(PySide2Tools_FIND_REQUIRED)
        if (NOT PySide2Tools_FIND_QUIETLY)
                message(STATUS "PySide2 tools: not found.")
        endif (NOT PySide2Tools_FIND_QUIETLY)
    endif(PySide2Tools_FIND_REQUIRED)
endif(PYSIDE2RCCBINARY AND PYSIDE2UICBINARY)
