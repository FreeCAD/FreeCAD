macro(SetupShibokenAndPyside)
# -------------------------------- Shiboken/PySide ------------------------

    option(FREECAD_USE_SHIBOKEN "Links to the shiboken library at build time. If OFF its Python module is imported at runtime" ON)
    option(FREECAD_USE_PYSIDE "Links to the PySide libraries at build time." ON)

    if(DEFINED MACPORTS_PREFIX)
        find_package(Shiboken REQUIRED HINTS "${PYTHON_LIBRARY_DIR}/cmake")
        find_package(PySide REQUIRED HINTS "${PYTHON_LIBRARY_DIR}/cmake")
    endif(DEFINED MACPORTS_PREFIX)

    if(FREECAD_QT_MAJOR_VERSION EQUAL 5)
        set(SHIBOKEN_MAJOR_VERSION 2)
        set(PYSIDE_MAJOR_VERSION 2)
    else()
        set(SHIBOKEN_MAJOR_VERSION ${FREECAD_QT_MAJOR_VERSION})
        set(PYSIDE_MAJOR_VERSION ${FREECAD_QT_MAJOR_VERSION})
    endif()


    # Shiboken2Config.cmake may explicitly set CMAKE_BUILD_TYPE to Release which causes
    # CMake to fail to create Makefiles for a debug build.
    # So as a workaround we save and restore the value after checking for Shiboken2.
    set (SAVE_BUILD_TYPE ${CMAKE_BUILD_TYPE})
    find_package(Shiboken${SHIBOKEN_MAJOR_VERSION} QUIET)
    set (CMAKE_BUILD_TYPE ${SAVE_BUILD_TYPE})
    if (Shiboken${SHIBOKEN_MAJOR_VERSION}_FOUND)
        # Shiboken config file was found but it may use the wrong Python version
        # Try to get the matching config suffix and repeat finding the package
        set(SHIBOKEN_PATTERN .cpython-${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR})

        file(GLOB SHIBOKEN_CONFIG "${Shiboken${SHIBOKEN_MAJOR_VERSION}_DIR}/Shiboken${SHIBOKEN_MAJOR_VERSION}Config${SHIBOKEN_PATTERN}*.cmake")
        if (SHIBOKEN_CONFIG)
            get_filename_component(SHIBOKEN_CONFIG_SUFFIX ${SHIBOKEN_CONFIG} NAME)
            string(SUBSTRING ${SHIBOKEN_CONFIG_SUFFIX} 15 -1 SHIBOKEN_CONFIG_SUFFIX)
            string(REPLACE ".cmake" "" PYTHON_CONFIG_SUFFIX ${SHIBOKEN_CONFIG_SUFFIX})
            message(STATUS "PYTHON_CONFIG_SUFFIX: ${PYTHON_CONFIG_SUFFIX}")
            find_package(Shiboken${SHIBOKEN_MAJOR_VERSION} QUIET)
        endif()

        if(TARGET Shiboken6::libshiboken AND SHIBOKEN_MAJOR_VERSION EQUAL 6)
            set_target_properties(Shiboken6::libshiboken PROPERTIES INTERFACE_COMPILE_DEFINITIONS "NDEBUG")
        endif()
    endif()

    # pyside2 changed its cmake files, this is the dance we have
    # to dance to be compatible with the old (<5.12) and the new versions (>=5.12)
    if(NOT SHIBOKEN_INCLUDE_DIR AND TARGET Shiboken${SHIBOKEN_MAJOR_VERSION}::libshiboken)
        get_property(SHIBOKEN_INCLUDE_DIR TARGET Shiboken${SHIBOKEN_MAJOR_VERSION}::libshiboken PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    endif(NOT SHIBOKEN_INCLUDE_DIR AND TARGET Shiboken${SHIBOKEN_MAJOR_VERSION}::libshiboken)

    if(NOT SHIBOKEN_INCLUDE_DIR)
        find_pip_package(Shiboken${SHIBOKEN_MAJOR_VERSION})
        if (Shiboken${SHIBOKEN_MAJOR_VERSION}_FOUND)
            set(SHIBOKEN_INCLUDE_DIR ${Shiboken${SHIBOKEN_MAJOR_VERSION}_INCLUDE_DIRS})
            set(SHIBOKEN_LIBRARY ${Shiboken${SHIBOKEN_MAJOR_VERSION}_LIBRARIES})
        endif()
    endif()

    find_package(PySide${PYSIDE_MAJOR_VERSION} QUIET)

    if(${PYSIDE_MAJOR_VERSION} EQUAL 2)
        # Our internal FindPySide6.cmake file already provides these for PySide6
        if(NOT PYSIDE_INCLUDE_DIR AND TARGET PySide${PYSIDE_MAJOR_VERSION}::pyside${PYSIDE_MAJOR_VERSION})
            get_property(PYSIDE_INCLUDE_DIR TARGET PySide${PYSIDE_MAJOR_VERSION}::pyside${PYSIDE_MAJOR_VERSION} PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
        endif(NOT PYSIDE_INCLUDE_DIR AND TARGET PySide${PYSIDE_MAJOR_VERSION}::pyside${PYSIDE_MAJOR_VERSION})

        if(NOT PYSIDE_INCLUDE_DIR)
            find_pip_package(PySide${PYSIDE_MAJOR_VERSION})
            if (PySide${PYSIDE_MAJOR_VERSION}_FOUND)
                set(PYSIDE_INCLUDE_DIR ${PySide${PYSIDE_MAJOR_VERSION}_INCLUDE_DIRS})
                set(PYSIDE_LIBRARY ${PySide${PYSIDE_MAJOR_VERSION}_LIBRARIES})
            endif()
        endif()
    endif()

    find_package(PySide${PYSIDE_MAJOR_VERSION}Tools QUIET) # PySide utilities (uic & rcc executables)
    if(NOT PYSIDE_TOOLS_FOUND)
        message("=======================\n"
                "PySide${PYSIDE_MAJOR_VERSION}Tools not found.\n"
                "=======================\n")
    endif()

    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Ext/PySide)
    file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/__init__.py "# PySide wrapper\n"
                                "from PySide${PYSIDE_MAJOR_VERSION} import __version__\n"
                                "from PySide${PYSIDE_MAJOR_VERSION} import __version_info__\n")
    file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtCore.py "from PySide${PYSIDE_MAJOR_VERSION}.QtCore import *\n\n"
                                "#QCoreApplication.CodecForTr=0\n"
                                "#QCoreApplication.UnicodeUTF8=1\n")
    file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtNetwork.py  "from PySide${PYSIDE_MAJOR_VERSION}.QtNetwork import *\n")
    if(BUILD_GUI)
        file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtGui.py  "from PySide${PYSIDE_MAJOR_VERSION}.QtGui import *\n"
                                    "from PySide${PYSIDE_MAJOR_VERSION}.QtWidgets import *\n"
                                    "QHeaderView.setResizeMode = QHeaderView.setSectionResizeMode\n")
        file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtSvg.py  "from PySide${PYSIDE_MAJOR_VERSION}.QtSvg import *\n")
        file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtUiTools.py  "from PySide${PYSIDE_MAJOR_VERSION}.QtUiTools import *\n")
        file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtWidgets.py  "from PySide${PYSIDE_MAJOR_VERSION}.QtWidgets import *\n")
        if(PYSIDE_MAJOR_VERSION LESS 6)
            file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtSvgWidgets.py  "from PySide${PYSIDE_MAJOR_VERSION}.QtSvg import QGraphicsSvgItem\n"
                                                                       "from PySide${PYSIDE_MAJOR_VERSION}.QtSvg import QSvgWidget\n")
            file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtWebEngineWidgets.py  "from PySide${PYSIDE_MAJOR_VERSION}.QtWebEngineWidgets import *\n")
        else()
            file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtSvgWidgets.py  "from PySide${PYSIDE_MAJOR_VERSION}.QtSvgWidgets import *\n")
            file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtWebEngineWidgets.py  "from PySide${PYSIDE_MAJOR_VERSION}.QtWebEngineWidgets import *\n"
                                                                              "from PySide${PYSIDE_MAJOR_VERSION}.QtWebEngineCore import QWebEnginePage\n")
        endif()
    endif()

    if(APPLE AND NOT BUILD_WITH_CONDA)
        install(
        DIRECTORY
            ${CMAKE_BINARY_DIR}/Ext/PySide
        DESTINATION
            MacOS
        )
    else()
        install(
        DIRECTORY
            ${CMAKE_BINARY_DIR}/Ext/PySide
        DESTINATION
            Ext
        )
    endif()

    # If shiboken cannot be found the build option will be set to OFF
    if(NOT SHIBOKEN_INCLUDE_DIR)
        message(WARNING "Shiboken${PYSIDE_MAJOR_VERSION} include files not found, FREECAD_USE_SHIBOKEN automatically set to OFF")
        set(FREECAD_USE_SHIBOKEN OFF)
    endif()

    # Now try to import the shiboken Python module and print a warning if it can't be loaded
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import shiboken${SHIBOKEN_MAJOR_VERSION}"
        RESULT_VARIABLE FAILURE
        OUTPUT_VARIABLE PRINT_OUTPUT
    )

    if(FAILURE)
        message(WARNING
                "==================================\n"
                "Shiboken${SHIBOKEN_MAJOR_VERSION} Python module not found.\n"
                "==================================\n")
    else()
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import shiboken${SHIBOKEN_MAJOR_VERSION};print(shiboken${SHIBOKEN_MAJOR_VERSION}.__version__, end='')"
            RESULT_VARIABLE FAILURE
            OUTPUT_VARIABLE Shiboken_VERSION
        )
    endif()

    # If PySide cannot be found the build option will be set to OFF
    if(NOT PYSIDE_INCLUDE_DIR)
        message(WARNING "PySide${PYSIDE_MAJOR_VERSION} include files not found, FREECAD_USE_PYSIDE automatically set to OFF")
        set(FREECAD_USE_PYSIDE OFF)
    endif()

    # Independent of the build option PySide modules must be loaded at runtime. Print a warning if it fails.
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import PySide${PYSIDE_MAJOR_VERSION};import os;print(os.path.dirname(PySide${PYSIDE_MAJOR_VERSION}.__file__), end='')"
        RESULT_VARIABLE FAILURE
        OUTPUT_VARIABLE PRINT_OUTPUT
    )
    if(FAILURE)
        message(WARNING
                "================================\n"
                "PySide${PYSIDE_MAJOR_VERSION} Python module not found.\n"
                "================================\n")
    else()
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import PySide${PYSIDE_MAJOR_VERSION};print(PySide${PYSIDE_MAJOR_VERSION}.__version__, end='')"
            RESULT_VARIABLE FAILURE
            OUTPUT_VARIABLE PySide_VERSION
        )
        message(STATUS "PySide ${PySide_VERSION} Python module found at ${PRINT_OUTPUT}.\n")
    endif()

endmacro(SetupShibokenAndPyside)


# Macros similar to FindQt4.cmake's WRAP_UI and WRAP_RC, for the automatic generation of Python
# code from Qt4's user interface ('.ui') and resource ('.qrc') files. These macros are called:
# - PYSIDE_WRAP_UI
# - PYSIDE_WRAP_RC

MACRO(PYSIDE_WRAP_UI outfiles)
  if (NOT PYSIDE_UIC_EXECUTABLE)
    message(FATAL_ERROR "Qt uic is required for generating ${ARGN}")
  endif()
  FOREACH(it ${ARGN})
    GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.py)
    if(WIN32 OR APPLE)
        ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
          COMMAND ${PYSIDE_UIC_EXECUTABLE} ${UICOPTIONS} ${infile} -o ${outfile}
          MAIN_DEPENDENCY ${infile}
        )
    else()
        # Especially on Open Build Service we don't want changing date like
        # pyside2-uic generates in comments at beginning., which is why
        # we follow the tool command with a POSIX-friendly sed.
        ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
          COMMAND "${PYSIDE_UIC_EXECUTABLE}" ${UICOPTIONS} "${infile}" -o "${outfile}"
          COMMAND sed "/^# /d" "${outfile}" >"${outfile}.tmp" && mv "${outfile}.tmp" "${outfile}"
          MAIN_DEPENDENCY "${infile}"
        )
    endif()
    list(APPEND ${outfiles} ${outfile})
  ENDFOREACH(it)
ENDMACRO (PYSIDE_WRAP_UI)

MACRO(PYSIDE_WRAP_RC outfiles)
  if (NOT PYSIDE_RCC_EXECUTABLE)
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
          COMMAND ${PYSIDE_RCC_EXECUTABLE} ${RCCOPTIONS} ${infile} -o ${outfile}
          MAIN_DEPENDENCY ${infile}
        )
    else()
        # Especially on Open Build Service we don't want changing date like
        # pyside-rcc generates in comments at beginning, which is why
        # we follow the tool command with in-place sed.
        ADD_CUSTOM_COMMAND(OUTPUT "${outfile}"
          COMMAND "${PYSIDE_RCC_EXECUTABLE}" ${RCCOPTIONS} "${infile}" ${PY_ATTRIBUTE} -o "${outfile}"
          # The line below sometimes catches unwanted lines too - but there is no date in the file
          # anymore with Qt5 RCC, so commenting it out for now...
          #COMMAND sed "/^# /d" "${outfile}" >"${outfile}.tmp" && mv "${outfile}.tmp" "${outfile}"
          MAIN_DEPENDENCY "${infile}"
        )
    endif()
    list(APPEND ${outfiles} ${outfile})
  ENDFOREACH(it)
ENDMACRO (PYSIDE_WRAP_RC)
