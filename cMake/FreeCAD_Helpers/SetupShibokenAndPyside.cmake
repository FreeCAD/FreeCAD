macro(SetupShibokenAndPyside)
# -------------------------------- Shiboken/PySide ------------------------

    option(FREECAD_USE_SHIBOKEN "Links to the shiboken library at build time. If OFF its Python module is imported at runtime" ON)
    option(FREECAD_USE_PYSIDE "Links to the PySide libraries at build time." ON)
    option(FREECAD_BINARY_PYTHON_RC_FILES "Put Qt resource file binaries outside of their corresponding Python files." ON)
    mark_as_advanced(FREECAD_BINARY_PYTHON_RC_FILES)

    if(DEFINED MACPORTS_PREFIX)
        find_package(Shiboken REQUIRED HINTS "${PYTHON_LIBRARY_DIR}/cmake")
        find_package(PySide REQUIRED HINTS "${PYTHON_LIBRARY_DIR}/cmake")
    endif()

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
    set(SAVE_BUILD_TYPE ${CMAKE_BUILD_TYPE})
    find_package(Shiboken${SHIBOKEN_MAJOR_VERSION} QUIET)
    set(CMAKE_BUILD_TYPE ${SAVE_BUILD_TYPE})
    if(Shiboken${SHIBOKEN_MAJOR_VERSION}_FOUND)
        # Shiboken config file was found but it may use the wrong Python version
        # Try to get the matching config suffix and repeat finding the package
        set(SHIBOKEN_PATTERN .cpython-${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR})

        file(GLOB SHIBOKEN_CONFIG "${Shiboken${SHIBOKEN_MAJOR_VERSION}_DIR}/Shiboken${SHIBOKEN_MAJOR_VERSION}Config${SHIBOKEN_PATTERN}*.cmake")
        if(SHIBOKEN_CONFIG)
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
    endif()

    if(NOT SHIBOKEN_INCLUDE_DIR)
        find_pip_package(Shiboken${SHIBOKEN_MAJOR_VERSION})
        if(Shiboken${SHIBOKEN_MAJOR_VERSION}_FOUND)
            set(SHIBOKEN_INCLUDE_DIR ${Shiboken${SHIBOKEN_MAJOR_VERSION}_INCLUDE_DIRS})
            set(SHIBOKEN_LIBRARY ${Shiboken${SHIBOKEN_MAJOR_VERSION}_LIBRARIES})
        endif()
    endif()

    find_package(PySide${PYSIDE_MAJOR_VERSION} QUIET)

    if(${PYSIDE_MAJOR_VERSION} EQUAL 2)
        # Our internal FindPySide6.cmake file already provides these for PySide6
        if(NOT PYSIDE_INCLUDE_DIR AND TARGET PySide${PYSIDE_MAJOR_VERSION}::pyside${PYSIDE_MAJOR_VERSION})
            get_property(PYSIDE_INCLUDE_DIR TARGET PySide${PYSIDE_MAJOR_VERSION}::pyside${PYSIDE_MAJOR_VERSION} PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
        endif()

        if(NOT PYSIDE_INCLUDE_DIR)
            find_pip_package(PySide${PYSIDE_MAJOR_VERSION})
            if(PySide${PYSIDE_MAJOR_VERSION}_FOUND)
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

endmacro()

file(
    GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/cMake/FreeCAD_Helpers/PysideQtRccGen.cmake"
    CONTENT [[
cmake_minimum_required(VERSION 3.22.0)
file(WRITE "${OUT}"
    "# Auto-generated loading code for ${RCCBIN}\n"
    "from pathlib import Path\n"
    "from PySide${PYSIDE_MAJOR_VERSION}.QtCore import QResource\n"
    "QResource.registerResource(str(Path(__file__).absolute().parent / \"${RCCBIN}\"))\n"
)
]])

# Function to generate Python files for Qt resources
# Usage -
#   PYSIDE_WRAP_RC(
#     [RELATIVE outfiles_relative]
#     [ABSOLUTE outfiles_absolute]
#     [RESOURCES resources]
#   )
# Arguments -
#   outfiles_relative - name of the list variable to append generated files' relative paths to
#   outfiles_absolute - name of the list variable to append generated files' absolute paths to
#   resources         - a list of .rc files to compile into Python Qt resources
# The generated files are created in ${CMAKE_CURRENT_BINARY_DIR} and their name depends on the
# respective resource files passed in ARGN.
function(PYSIDE_WRAP_RC)
    set(options)
    set(oneValueArgs
        RELATIVE
        ABSOLUTE
    )
    set(multiValueArgs
        RESOURCES
    )
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(NOT PYSIDE_RCC_EXECUTABLE)
        message(FATAL_ERROR "Qt rcc is required for generating ${ARGN}")
    endif()
    if(("${arg_RELATIVE}" STREQUAL "") AND ("${arg_ABSOLUTE}" STREQUAL ""))
        message(FATAL_ERROR "Neither RELATIVE or ABSOLUTE lists were specified")
    endif()
    foreach(it ${arg_RESOURCES})
        get_filename_component(rcpy_base ${it} NAME_WE)
        get_filename_component(infile ${it} ABSOLUTE)
        file(RELATIVE_PATH infile_relative_to_root "${CMAKE_SOURCE_DIR}" "${infile}")
        set(rcpy_relative "${rcpy_base}_rc.py")
        set(rcpy "${CMAKE_CURRENT_BINARY_DIR}/${rcpy_relative}")

        if(FREECAD_BINARY_PYTHON_RC_FILES)
            set(rccbin_relative "${rcpy_base}_rc.rcc")
            set(rccbin "${CMAKE_CURRENT_BINARY_DIR}/${rccbin_relative}")
            add_custom_command(OUTPUT "${rcpy}" "${rccbin}"
                COMMAND "${PYSIDE_RCC_EXECUTABLE}" ${FREECAD_RCC_OPTIONS}
                        --binary "${infile}" -o "${rccbin}"
                COMMAND "${CMAKE_COMMAND}" -DOUT="${rcpy}" -DRCCBIN="${rccbin_relative}"
                        -DPYSIDE_MAJOR_VERSION="${PYSIDE_MAJOR_VERSION}"
                        -P "${CMAKE_BINARY_DIR}/cMake/FreeCAD_Helpers/PysideQtRccGen.cmake"
                MAIN_DEPENDENCY "${infile}"
                DEPENDS "${PYSIDE_RCC_EXECUTABLE}"
                        "${CMAKE_BINARY_DIR}/cMake/FreeCAD_Helpers/PysideQtRccGen.cmake"
                COMMENT "Generating binary Python Qt Resource files for ${infile_relative_to_root}"
            )
            if(NOT "${arg_RELATIVE}" STREQUAL "")
                list(APPEND ${arg_RELATIVE} "${rcpy_relative}" "${rccbin_relative}")
            endif()
            if(NOT "${arg_ABSOLUTE}" STREQUAL "")
                list(APPEND ${arg_ABSOLUTE} "${rcpy}" "${rccbin}")
            endif()
        else()
            add_custom_command(OUTPUT "${rcpy}"
                COMMAND "${PYSIDE_RCC_EXECUTABLE}" ${PYSIDE_RCC_OPTIONS} "${infile}" -o "${rcpy}"
                MAIN_DEPENDENCY "${infile}"
                DEPENDS "${PYSIDE_RCC_EXECUTABLE}"
                COMMENT "Generating Python Qt Resource file for ${infile_relative_to_root}"
            )
            if(NOT "${arg_RELATIVE}" STREQUAL "")
                list(APPEND ${arg_RELATIVE} "${rcpy_relative}")
            endif()
            if(NOT "${arg_ABSOLUTE}" STREQUAL "")
                list(APPEND ${arg_ABSOLUTE} "${rcpy}")
            endif()
        endif()
    endforeach()
    if(NOT "${arg_RELATIVE}" STREQUAL "")
        set(${arg_RELATIVE} "${${arg_RELATIVE}}" PARENT_SCOPE)
    endif()
    if(NOT "${arg_ABSOLUTE}" STREQUAL "")
        set(${arg_ABSOLUTE} "${${arg_ABSOLUTE}}" PARENT_SCOPE)
    endif()
endfunction()
