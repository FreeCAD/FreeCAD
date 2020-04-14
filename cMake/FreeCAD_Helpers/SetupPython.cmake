macro(SetupPython)
# -------------------------------- Python --------------------------------

    # http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=677598
    # Acceptable versions of Python
    set(Python_ADDITIONAL_VERSIONS "2.7")

    # For building on OS X
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT BUILD_WITH_CONDA)

        # If the user doesn't tell us which package manager they're using
        if(NOT DEFINED MACPORTS_PREFIX AND NOT DEFINED HOMEBREW_PREFIX)

            # Try to find MacPorts path
            find_program(MACPORTS_EXECUTABLE port)
            if(EXISTS ${MACPORTS_EXECUTABLE})
                string(REPLACE "/bin/port" ""
                       MACPORTS_PREFIX ${MACPORTS_EXECUTABLE})
                message(STATUS "Detected MacPorts install at ${MACPORTS_PREFIX}")
            endif(EXISTS ${MACPORTS_EXECUTABLE})

            # Try to find Homebrew path
            find_program(HOMEBREW_EXECUTABLE brew)
            if(EXISTS ${HOMEBREW_EXECUTABLE})
                string(REPLACE "/bin/brew" ""
                       HOMEBREW_PREFIX ${HOMEBREW_EXECUTABLE})
                message(STATUS "Detected Homebrew install at ${HOMEBREW_PREFIX}")
            endif()

        endif(NOT DEFINED MACPORTS_PREFIX AND NOT DEFINED HOMEBREW_PREFIX)

        # In case someone tries to shoot themselves in the foot
        if(DEFINED MACPORTS_PREFIX AND DEFINED HOMEBREW_PREFIX)
            message(SEND_ERROR
                    "Multiple package management systems detected - ")
            message(SEND_ERROR
                    "define either MACPORTS_PREFIX or HOMEBREW_PREFIX")

        # No package manager
        elseif(NOT DEFINED MACPORTS_PREFIX AND NOT DEFINED HOMEBREW_PREFIX)
            message(SEND_ERROR
                    "No package manager detected - install MacPorts or Homebrew")

        # The hopefully-normal case - one package manager identified
        else(DEFINED MACPORTS_PREFIX AND DEFINED HOMEBREW_PREFIX)

            # Construct a list like python;python2.9;python2.8;...
            set(Python_ADDITIONAL_VERSIONS_REV ${Python_ADDITIONAL_VERSIONS})
            list(REVERSE Python_ADDITIONAL_VERSIONS_REV)
            set(_PYTHON_NAMES "python")
            foreach(_PYTHON_VERSION IN LISTS Python_ADDITIONAL_VERSIONS_REV)
                list(APPEND _PYTHON_NAMES "python${_PYTHON_VERSION}")
            endforeach(_PYTHON_VERSION)

            # Find python in the package management systems, using names in that
            # list in decreasing priority.  Note that a manually specified
            # PYTHON_EXECUTABLE still has prescedence over this.
            find_program(PYTHON_EXECUTABLE
                         NAMES ${_PYTHON_NAMES}
                         PATHS ${MACPORTS_PREFIX} ${HOMEBREW_PREFIX}
                         PATH_SUFFIXES /bin
                         NO_DEFAULT_PATH)

        endif(DEFINED MACPORTS_PREFIX AND DEFINED HOMEBREW_PREFIX)

        # Warn user if we still only have the system Python
        string(FIND ${PYTHON_EXECUTABLE} "/usr/bin/python" _FIND_SYS_PYTHON)
        if(_FIND_SYS_PYTHON EQUAL 0)
            message(SEND_ERROR
                    "Only found the stock Python, that's probably bad.")
        endif(_FIND_SYS_PYTHON EQUAL 0)

        # Ask Python to tell us it's include directory, if nobody else has
        if(NOT DEFINED PYTHON_INCLUDE_DIR)
            execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
             "from distutils.sysconfig import get_python_inc;print(get_python_inc())"
                            OUTPUT_VARIABLE PYTHON_INCLUDE_DIR
                            RESULT_VARIABLE PYTHON_INCLUDE_DIR_RESULT
                            ERROR_QUIET)
            if(NOT PYTHON_INCLUDE_DIR_RESULT MATCHES 0)
                message(SEND_ERROR "Failed to determine PYTHON_INCLUDE_DIR")
            endif(NOT PYTHON_INCLUDE_DIR_RESULT MATCHES 0)
        endif(NOT DEFINED PYTHON_INCLUDE_DIR)

        # Similar for the Python library - there must be an easier way...
        if(NOT DEFINED PYTHON_LIBRARY)
            # Get the library path
            execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c
          "from distutils import sysconfig;print(sysconfig.get_config_var('LIBDIR'))"
                            OUTPUT_VARIABLE PYTHON_LIBRARY_DIR
                            RESULT_VARIABLE PYTHON_LIBRARY_DIR_RESULT
                            ERROR_QUIET)
            string(STRIP ${PYTHON_LIBRARY_DIR} PYTHON_LIBRARY_DIR)
            if(NOT PYTHON_LIBRARY_DIR_RESULT MATCHES 0)
                message(SEND_ERROR "Failed to determine PYTHON_LIBRARY")
            endif(NOT PYTHON_LIBRARY_DIR_RESULT MATCHES 0)

            # Get library filename - might not be safe to assume .dylib extension?
            execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c
                     "import sys;print('libpython%d.%d.dylib'%sys.version_info[0:2])"
                            OUTPUT_VARIABLE PYTHON_LIBRARY_FILE
                            RESULT_VARIABLE PYTHON_LIBRARY_FILE_RESULT
                            ERROR_QUIET)
            string(STRIP ${PYTHON_LIBRARY_FILE} PYTHON_LIBRARY_FILE)
            if(NOT PYTHON_LIBRARY_FILE_RESULT MATCHES 0)
                message(SEND_ERROR "Failed to determine PYTHON_LIBRARY")
            endif(NOT PYTHON_LIBRARY_FILE_RESULT MATCHES 0)

            set(PYTHON_LIBRARY "${PYTHON_LIBRARY_DIR}/${PYTHON_LIBRARY_FILE}")

        else(NOT DEFINED PYTHON_LIBRARY)
            # Used on MacPorts systems for finding Shiboken and PySide
            # TODO: When we start requiring minimum CMake version above
            # 2.8.11, change PATH below to DIRECTORY
            get_filename_component(PYTHON_LIBRARY_DIR ${PYTHON_LIBRARY} PATH)
        endif(NOT DEFINED PYTHON_LIBRARY)


    endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT BUILD_WITH_CONDA)

    find_package(PythonInterp REQUIRED)
    
    # Issue in cmake prevents finding pythonlibs 3.x when python 2.7 is present
    # setting NOTFOUND here resolves the issue
    set(PYTHON_INCLUDE_DIR "NOTFOUND")
    set(PYTHON_LIBRARY "NOTFOUND")
    
    set(Python_ADDITIONAL_VERSIONS ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
    if (NOT DEFINED PYTHON_VERSION_STRING)
        find_package(PythonLibs REQUIRED)
    else (NOT DEFINED PYTHON_VERSION_STRING)
        find_package(PythonLibs ${PYTHON_VERSION_STRING} EXACT)
    endif(NOT DEFINED PYTHON_VERSION_STRING)


    if(NOT PYTHONLIBS_FOUND)
        message(FATAL_ERROR "=================================\n"
                            "Python not found, install Python!\n"
                            "=================================\n")
    else(NOT PYTHONLIBS_FOUND)
        # prevent python3 lower than 3.3 (not enough utf8<->unicode tools)
        if(PYTHON_VERSION_MAJOR EQUAL 3)
            if(PYTHON_VERSION_MINOR LESS 4)
                message(FATAL_ERROR "To build FreeCAD with Python3, you need at least version 3.4\n")
            endif(PYTHON_VERSION_MINOR LESS 4)
        endif(PYTHON_VERSION_MAJOR EQUAL 3)
    endif(NOT PYTHONLIBS_FOUND)

endmacro(SetupPython)
