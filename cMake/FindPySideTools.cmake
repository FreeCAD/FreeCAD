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

if(WIN32)
	FIND_PROGRAM(PYSIDEUIC4BINARY pyside-uic
		HINTS ${PYSIDE_BIN_DIR}
	)
	FIND_PROGRAM(PYSIDERCC4BINARY pyside-rcc
		HINTS ${PYSIDE_BIN_DIR}
	)
else(WIN32)
    if(APPLE)
    	#set (PYTHON_BIN_DIR /opt/local/Library/Frameworks/Python.framework/Versions/2.7/bin )
    	FIND_PROGRAM( PYSIDEUIC4BINARY PYSIDEUIC4
    		HINTS ${PYSIDE_BIN_DIR}
    		)
    	FIND_PROGRAM(PYSIDERCC4BINARY PYSIDERCC4
    		HINTS ${PYSIDE_BIN_DIR}
    		)
    else(APPLE)
        FIND_PROGRAM(PYSIDEUIC4BINARY pyside-uic)
        FIND_PROGRAM(PYSIDERCC4BINARY pyside-rcc)
    endif(APPLE)
endif(WIN32)
#message(STATUS "PYSIDEUIC4BINARY ${PYSIDEUIC4BINARY}" )
#message(STATUS "PYSIDERCC4BINARY ${PYSIDERCC4BINARY}" )

MACRO(PYSIDE_WRAP_UI outfiles)
  FOREACH(it ${ARGN})
    GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.py)
    #ADD_CUSTOM_TARGET(${it} ALL
    #  DEPENDS ${outfile}
    #)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
      COMMAND ${PYSIDEUIC4BINARY} ${infile} -o ${outfile}
      MAIN_DEPENDENCY ${infile}
    )
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
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
      COMMAND ${PYSIDERCC4BINARY} ${infile} -o ${outfile}
      MAIN_DEPENDENCY ${infile}
    )
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH(it)
ENDMACRO (PYSIDE_WRAP_RC)

IF(EXISTS ${PYSIDEUIC4BINARY} AND EXISTS ${PYSIDERCC4BINARY})
   set(PYSIDE_TOOLS_FOUND TRUE)
ENDIF(EXISTS ${PYSIDEUIC4BINARY} AND EXISTS ${PYSIDERCC4BINARY})

if(PYSIDE_TOOLS_FOUND)
  if(NOT PYSIDE_TOOLS_FOUND_QUIETLY)
    message(STATUS "Found PySide Tools: ${PYSIDEUIC4BINARY}, ${PYSIDERCC4BINARY}")
  endif(NOT PYSIDE_TOOLS_FOUND_QUIETLY)
endif(PYSIDE_TOOLS_FOUND)
