macro(SetupPython)
# -------------------------------- Python --------------------------------

    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

    if (${Python3_VERSION} VERSION_LESS "3.10")
         message(FATAL_ERROR "To build FreeCAD you need at least Python 3.10\n")
    endif()

    # For backwards compatibility with old CMake scripts
    # See: https://github.com/FreeCAD/FreeCAD/pull/19635#issuecomment-2725612407
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    set(PYTHON_LIBRARIES ${Python3_LIBRARIES})
    set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
    set(PYTHON_LIBRARY_DIRS ${Python3_LIBRARY_DIRS})
    set(PYTHON_VERSION_STRING ${Python3_VERSION})
    set(PYTHON_VERSION_MAJOR ${Python3_VERSION_MAJOR})
    set(PYTHON_VERSION_MINOR ${Python3_VERSION_MINOR})
    set(PYTHON_VERSION_PATCH ${Python3_VERSION_PATCH})
    set(PYTHONINTERP_FOUND ${Python3_Interpreter_FOUND})

endmacro(SetupPython)
