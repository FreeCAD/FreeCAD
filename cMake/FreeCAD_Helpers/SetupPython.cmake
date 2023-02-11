macro(SetupPython)
# -------------------------------- Python --------------------------------

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
