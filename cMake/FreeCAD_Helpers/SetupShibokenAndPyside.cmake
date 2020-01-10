macro(SetupShibokenAndPyside)
# -------------------------------- Shiboken/PySide ------------------------

    if(BUILD_QT5)
        # set(PYTHON_SUFFIX -python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
        # Below are two variables that can be left empty for standard python 3 version,
        # but must be set by the user for different python versions such as 2.7, 3.5 ...
        if (PYTHON_VERSION_MAJOR LESS 3)
            set(PYTHON_CONFIG_SUFFIX -python2.7 CACHE STRING "Shiboken cmake file suffix. If left empty, system default will be used: <ShibokenConfigPYTHON_CONFIG_SUFFIX.cmake>")
            set(PYTHON_BASENAME -python2.7 CACHE STRING "Same as PYTHON_SUFFIX but for PySide. If left empty, PYTHON_SUFFIX will be used: <PySideConfigPYTHON_BASENAME.cmake>")
        else()
            #set(PYTHON_CONFIG_SUFFIX "" CACHE STRING "Shiboken cmake file suffix. If left empty, system default will be used: <ShibokenConfigPYTHON_CONFIG_SUFFIX.cmake>")
            #set(PYTHON_BASENAME "" CACHE STRING "Same as PYTHON_SUFFIX but for PySide. If left empty, PYTHON_SUFFIX will be used: <PySideConfigPYTHON_BASENAME.cmake>")
        endif()

        if(DEFINED MACPORTS_PREFIX)
            find_package(Shiboken REQUIRED HINTS "${PYTHON_LIBRARY_DIR}/cmake")
            find_package(PySide REQUIRED HINTS "${PYTHON_LIBRARY_DIR}/cmake")
        endif(DEFINED MACPORTS_PREFIX)

        # Shiboken2Config.cmake may explicitly set CMAKE_BUILD_TYPE to Release which causes
        # CMake to fail to create Makefiles for a debug build.
        # So as a workaround we save and restore the value after checking for Shiboken2.
        set (SAVE_BUILD_TYPE ${CMAKE_BUILD_TYPE})
        find_package(Shiboken2 QUIET)# REQUIRED
        set (CMAKE_BUILD_TYPE ${SAVE_BUILD_TYPE})
        if (Shiboken2_FOUND)
            # Shiboken2 config file was found but it may use the wrong Python version
            # Try to get the matching config suffix and repeat finding the package
            if (PYTHON_VERSION_MAJOR LESS 3)
                set(SHIBOKEN_PATTERN -python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
            else()
                set(SHIBOKEN_PATTERN .cpython-${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR})
            endif()

            file(GLOB SHIBOKEN_CONFIG "${Shiboken2_DIR}/Shiboken2Config${SHIBOKEN_PATTERN}*.cmake")
            if (SHIBOKEN_CONFIG)
                get_filename_component(SHIBOKEN_CONFIG_SUFFIX ${SHIBOKEN_CONFIG} NAME)
                string(SUBSTRING ${SHIBOKEN_CONFIG_SUFFIX} 15 -1 SHIBOKEN_CONFIG_SUFFIX)
                string(REPLACE ".cmake" "" PYTHON_CONFIG_SUFFIX ${SHIBOKEN_CONFIG_SUFFIX})
                message(STATUS "PYTHON_CONFIG_SUFFIX: ${PYTHON_CONFIG_SUFFIX}")
                find_package(Shiboken2 QUIET)
            endif()
        endif()

        # pyside2 changed it's cmake files, this is the dance we have
        # to dance to be compatible with the old (<5.12) and the new versions (>=5.12)
        if(NOT SHIBOKEN_INCLUDE_DIR AND TARGET Shiboken2::libshiboken)
                get_property(SHIBOKEN_INCLUDE_DIR TARGET Shiboken2::libshiboken PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
        endif(NOT SHIBOKEN_INCLUDE_DIR AND TARGET Shiboken2::libshiboken)

        if(NOT SHIBOKEN_INCLUDE_DIR)
            message("====================\n"
                    "shiboken2 not found.\n"
                    "====================\n")
        endif(NOT SHIBOKEN_INCLUDE_DIR)

        find_package(PySide2 QUIET)# REQUIRED

        if(NOT PYSIDE_INCLUDE_DIR AND TARGET PySide2::pyside2)
                get_property(PYSIDE_INCLUDE_DIR TARGET PySide2::pyside2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
        endif(NOT PYSIDE_INCLUDE_DIR AND TARGET PySide2::pyside2)

        if(NOT PYSIDE_INCLUDE_DIR)
            message("==================\n"
                    "PySide2 not found.\n"
                    "==================\n")
        endif(NOT PYSIDE_INCLUDE_DIR)

        find_package(PySide2Tools QUIET) #REQUIRED # PySide2 utilities (pyside2-uic & pyside2-rcc)
        if(NOT PYSIDE2_TOOLS_FOUND)
            message("=======================\n"
                    "PySide2Tools not found.\n"
                    "=======================\n")
        endif(NOT PYSIDE2_TOOLS_FOUND)

        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Ext/PySide)
        file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/__init__.py "# PySide wrapper\n"
                                                              "from PySide2 import __version__\n"
                                                              "from PySide2 import __version_info__\n")
        file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtCore.py "from PySide2.QtCore import *\n\n"
                                                            "#QCoreApplication.CodecForTr=0\n"
                                                            "#QCoreApplication.UnicodeUTF8=1\n")
        file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtGui.py  "from PySide2.QtGui import *\n"
                                                            "from PySide2.QtWidgets import *\n"
                                                            "QHeaderView.setResizeMode = QHeaderView.setSectionResizeMode\n")
        file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtSvg.py  "from PySide2.QtSvg import *\n")
        file(WRITE ${CMAKE_BINARY_DIR}/Ext/PySide/QtUiTools.py  "from PySide2.QtUiTools import *\n")

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
    else(BUILD_QT5)
        # set(PYTHON_SUFFIX -python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
        # Below are two variables that can be left empty for standard python 3 version,
        # but must be set by the user for different python versions such as 2.7, 3.5 ...
        if (PYTHON_VERSION_MAJOR LESS 3)
            set(PYTHON_CONFIG_SUFFIX -python2.7 CACHE STRING "Shiboken cmake file suffix. If left empty, system default will be used: <ShibokenConfigPYTHON_CONFIG_SUFFIX.cmake>")
            set(PYTHON_BASENAME -python2.7 CACHE STRING "Same as PYTHON_SUFFIX but for PySide. If left empty, PYTHON_SUFFIX will be used: <PySideConfigPYTHON_BASENAME.cmake>")
        else()
            set(PYTHON_CONFIG_SUFFIX "" CACHE STRING "Shiboken cmake file suffix. If left empty, system default will be used: <ShibokenConfigPYTHON_CONFIG_SUFFIX.cmake>")
            set(PYTHON_BASENAME "" CACHE STRING "Same as PYTHON_SUFFIX but for PySide. If left empty, PYTHON_SUFFIX will be used: <PySideConfigPYTHON_BASENAME.cmake>")
        endif()

        if(DEFINED MACPORTS_PREFIX)
            find_package(Shiboken REQUIRED HINTS "${PYTHON_LIBRARY_DIR}/cmake")
            find_package(PySide REQUIRED HINTS "${PYTHON_LIBRARY_DIR}/cmake")
        endif(DEFINED MACPORTS_PREFIX)

        find_package(Shiboken)# REQUIRED
        if(NOT SHIBOKEN_INCLUDE_DIR)
            message(FATAL_ERROR "===================\n"
                                "shiboken not found.\n"
                                "===================\n")
        endif(NOT SHIBOKEN_INCLUDE_DIR)

        find_package(PySide)# REQUIRED
        if(NOT PYSIDE_INCLUDE_DIR)
            message(FATAL_ERROR "=================\n"
                                "PySide not found.\n"
                                "=================\n")
        endif(NOT PYSIDE_INCLUDE_DIR)

        find_package(PySideTools REQUIRED) # PySide utilities (pyside-uic & pyside-rcc)
    endif(BUILD_QT5)

endmacro(SetupShibokenAndPyside)
