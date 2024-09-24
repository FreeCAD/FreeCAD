# The Qt for Python project officially recommends using pip to install PySide, so we expect to find PySide in the
# site-packages directory. The library will be called "PySide6.abi3.*", and there will be an "include" directory inside
# the site-packages/PySide6. Over time some distros may provide custom versions, so we also support using a more normal
# cMake find_package() call

find_package(PySide6 CONFIG QUIET)
if(NOT PySide6_FOUND)
    if(NOT PySide6_INCLUDE_DIR AND TARGET PySide6::pyside6)
        get_property(PySide6_INCLUDE_DIR TARGET PySide6::pyside6 PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    endif()

    if(NOT PySide6_INCLUDE_DIR)
        find_pip_package(PySide6)
        if (PySide6_FOUND)
            set(PYSIDE_INCLUDE_DIR ${PySide6_INCLUDE_DIRS} CACHE INTERNAL "")
            set(PYSIDE_LIBRARY ${PySide6_LIBRARIES} CACHE INTERNAL "")
            set(PYSIDE_FOUND TRUE CACHE BOOL OFF)
            set(PYSIDE_MAJOR_VERSION 6 CACHE INTERNAL 6)
        endif()
    endif()

endif()
