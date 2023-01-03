macro(SetupPython)
# -------------------------------- Python --------------------------------

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

    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

    # For backwards compatibility with old CMake scripts
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    set(PYTHON_LIBRARIES ${Python3_LIBRARIES})
    set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
    set(PYTHON_LIBRARY_DIRS ${Python3_LIBRARY_DIRS})
    set(PYTHON_VERSION_STRING ${Python3_VERSION})
    set(PYTHON_VERSION_MAJOR ${Python3_VERSION_MAJOR})
    set(PYTHON_VERSION_MINOR ${Python3_VERSION_MINOR})
    set(PYTHON_VERSION_PATCH ${Python3_VERSION_PATCH})
    set(PYTHONINTERP_FOUND ${Python3_Interpreter_FOUND})

    if (${PYTHON_VERSION_STRING} VERSION_LESS "3.8")
         message(FATAL_ERROR "To build FreeCAD you need at least Python 3.8\n")
    endif()

endmacro(SetupPython)
