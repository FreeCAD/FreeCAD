# Try to find PySide utilities, PYSIDEUIC4 and PYSIDERCC4:
# PYSIDEUIC4BINARY - Location of PYSIDEUIC4 executable
# PYSIDERCC4BINARY - Location of PYSIDERCC4 executable
# PYSIDE_TOOLS_FOUND - PySide utilities found.

# Also provides macro similar to FindQt4.cmake's WRAP_UI and WRAP_RC,
# for the automatic generation of Python code from Qt4's user interface
# ('.ui') and resource ('.qrc') files. These macros are called:
# - PYSIDE_WRAP_UI
# - PYSIDE_WRAP_RC

IF(PYSIDEUIC4BINARY AND PYSIDERCC4BINARY)
  # Already in cache, be silent
  set(PYSIDE_TOOLS_FOUND_QUIETLY TRUE)
ENDIF(PYSIDEUIC4BINARY AND PYSIDERCC4BINARY)

if(WIN32 OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    #pyside tools are often in same location as python interpreter
    get_filename_component(PYTHON_BIN_DIR ${PYTHON_EXECUTABLE} PATH)
    set(PYSIDE_BIN_DIR ${PYTHON_BIN_DIR})
endif(WIN32 OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

FIND_PROGRAM(PYSIDEUIC4BINARY NAMES python2-pyside-uic pyside-uic pyside-uic-${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} HINTS ${PYSIDE_BIN_DIR})
FIND_PROGRAM(PYSIDERCC4BINARY NAMES pyside-rcc pyside-rcc-${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} HINTS ${PYSIDE_BIN_DIR})

MACRO(PYSIDE_WRAP_UI outfiles)
  FOREACH(it ${ARGN})
    GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.py)
    #ADD_CUSTOM_TARGET(${it} ALL
    #  DEPENDS ${outfile}
    #)
    if(WIN32)
        ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
          COMMAND ${PYSIDEUIC4BINARY} ${infile} -o ${outfile}
          MAIN_DEPENDENCY ${infile}
        )
    else(WIN32)
        # Especially on Open Build Service we don't want changing date like
        # pyside-uic generates in comments at beginning.
        EXECUTE_PROCESS(
          COMMAND ${PYSIDEUIC4BINARY} ${infile}
          COMMAND sed "/^# /d"
          OUTPUT_FILE ${outfile}
        )
    endif(WIN32)
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH(it)
ENDMACRO (PYSIDE_WRAP_UI)

MACRO(PYSIDE_WRAP_RC outfiles)
  FOREACH(it ${ARGN})
    GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/${outfile}_rc.py)
    #ADD_CUSTOM_TARGET(${it} ALL
    #  DEPENDS ${outfile}
    #)
    if(WIN32)
        ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
          COMMAND ${PYSIDERCC4BINARY} ${infile} -o ${outfile}
          MAIN_DEPENDENCY ${infile}
        )
    else(WIN32)
        # Especially on Open Build Service we don't want changing date like
        # pyside-rcc generates in comments at beginning.
        EXECUTE_PROCESS(
          COMMAND ${PYSIDERCC4BINARY} ${infile}
          COMMAND sed "/^# /d"
          OUTPUT_FILE ${outfile}
       )
    endif(WIN32)
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH(it)
ENDMACRO (PYSIDE_WRAP_RC)

IF(EXISTS ${PYSIDEUIC4BINARY} AND EXISTS ${PYSIDERCC4BINARY})
   set(PYSIDE_TOOLS_FOUND TRUE)
ENDIF(EXISTS ${PYSIDEUIC4BINARY} AND EXISTS ${PYSIDERCC4BINARY})

if(PYSIDERCC4BINARY AND PYSIDEUIC4BINARY)
    if (NOT PySideTools_FIND_QUIETLY)
        message(STATUS "Found PySide Tools: ${PYSIDEUIC4BINARY}, ${PYSIDERCC4BINARY}")
    endif (NOT PySideTools_FIND_QUIETLY)
else(PYSIDERCC4BINARY AND PYSIDEUIC4BINARY)
    if(PySideTools_FIND_REQUIRED)
        message(FATAL_ERROR "PySideTools could not be found, but are required.")
    else(PySideTools_FIND_REQUIRED)
        if (NOT PySideTools_FIND_QUIETLY)
                message(STATUS "PySideTools: not found.")
        endif (NOT PySideTools_FIND_QUIETLY)
    endif(PySideTools_FIND_REQUIRED)
endif(PYSIDERCC4BINARY AND PYSIDEUIC4BINARY)
