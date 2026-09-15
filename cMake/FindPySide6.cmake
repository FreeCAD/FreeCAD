# The Qt for Python project officially recommends using pip to install PySide,
# so we expect to find PySide in the site-packages directory.
# The library will be called "PySide6.abi3.*", and there will
# be an "include" directory inside the site-packages/PySide6.
# Over time some distros may provide custom versions, so we also support
# using a more normal cMake find_package() call

include(FindPackageHandleStandardArgs)

find_package(PySide6 CONFIG QUIET)

if(NOT PySide6_FOUND)
    find_pip_package(PySide6)
endif()

if(PySide6_FOUND)
    # verify PySide6 version matches Qt6 version (major.minor)
    if(PySide6_VERSION AND Qt6_VERSION)
        string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _qt6_major_minor "${Qt6_VERSION}")
        string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _pyside6_major_minor "${PySide6_VERSION}")

        message(STATUS "Qt version: ${Qt6_VERSION}")
        message(STATUS "PySide version: ${PySide6_VERSION}")

        if(NOT _qt6_major_minor STREQUAL _pyside6_major_minor)
            message(FATAL_ERROR
" --------------------------------------------------------
 Qt/PySide version mismatch!
 cmake found Qt: ${Qt6_VERSION}
 cmake found PySide: ${PySide6_VERSION}
 major.minor versions of Qt and PySide must match to avoid errors.
 Ensure CMAKE_PREFIX_PATH points to matching Qt and PySide6 installations.
 --------------------------------------------------------"
            )
        endif()

        message(STATUS "PySide/Qt version check passed (${_pyside6_major_minor})")
    endif()

    set(_pyside6_qtsvgwidgets_consumers)
    if(BUILD_BIM)
        list(APPEND _pyside6_qtsvgwidgets_consumers BUILD_BIM)
    endif()
    if(BUILD_MATERIAL)
        list(APPEND _pyside6_qtsvgwidgets_consumers BUILD_MATERIAL)
    endif()

    if(BUILD_GUI AND _pyside6_qtsvgwidgets_consumers)
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import PySide6.QtSvgWidgets"
            RESULT_VARIABLE PYSIDE6_QTSVGWIDGETS_IMPORT_FAILED
            ERROR_VARIABLE PYSIDE6_QTSVGWIDGETS_IMPORT_ERROR
            OUTPUT_QUIET
        )

        if(PYSIDE6_QTSVGWIDGETS_IMPORT_FAILED)
            list(JOIN _pyside6_qtsvgwidgets_consumers ", " _pyside6_qtsvgwidgets_consumer_list)
            string(STRIP "${PYSIDE6_QTSVGWIDGETS_IMPORT_ERROR}" PYSIDE6_QTSVGWIDGETS_IMPORT_ERROR)
            message(FATAL_ERROR
" --------------------------------------------------------
 PySide6.QtSvgWidgets Python module not found.
 This configuration enables modules that require it: ${_pyside6_qtsvgwidgets_consumer_list}
 BIM and Material use it for SVG previews.
 Install python3-pyside6.qtsvgwidgets or provide a PySide6 installation
 that includes QtSvgWidgets.
 Python executable: ${Python3_EXECUTABLE}
 Import error: ${PYSIDE6_QTSVGWIDGETS_IMPORT_ERROR}
 --------------------------------------------------------"
            )
        endif()
    endif()

    if(NOT PySide6_INCLUDE_DIRS AND TARGET PySide6::pyside6)
        get_property(PySide6_INCLUDE_DIRS TARGET PySide6::pyside6 PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    endif()

    find_package_handle_standard_args(PySide6
        REQUIRED_VARS PySide6_INCLUDE_DIRS
        VERSION_VAR PySide6_VERSION
    )

    # Also provide the old-style variables so we don't have to update everything yet
    set(PYSIDE_INCLUDE_DIR ${PySide6_INCLUDE_DIRS})
    set(PYSIDE_LIBRARY ${PySide6_LIBRARIES})
    set(PYSIDE_FOUND TRUE)
    set(PYSIDE_MAJOR_VERSION 6)
endif()
