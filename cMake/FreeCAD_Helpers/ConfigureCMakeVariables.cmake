macro(ConfigureCMakeVariables)
    # ================================================================================
    # Output directories for install target

    if(WIN32)
        set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Installation root directory")
        set(CMAKE_INSTALL_BINDIR bin CACHE PATH "Output directory for executables")
        set(CMAKE_INSTALL_DATADIR data CACHE PATH "Output directory for data and resource files")
        set(CMAKE_INSTALL_INCLUDEDIR include CACHE PATH "Output directory for header files")
        set(CMAKE_INSTALL_DOCDIR doc CACHE PATH "Output directory for documentation and license files")
        # Don't set it without manual adoption of LibDir variable in src/App/FreeCADInit.py
        set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")
    else()
        set(CMAKE_INSTALL_PREFIX "/usr/lib${LIB_SUFFIX}/freecad" CACHE PATH "Installation root directory")
        include(GNUInstallDirs)
    endif()

    set(PYCXX_INCLUDE_DIR
        "${CMAKE_SOURCE_DIR}/src" CACHE PATH
        "Path to the directory containing PyCXX's CXX/Config.hxx include file")
    set(PYCXX_SOURCE_DIR
        "${CMAKE_SOURCE_DIR}/src/CXX" CACHE PATH
        "Path to the directory containing PyCXX's cxxextensions.c source file")

    # used as compiler defines
    set(RESOURCEDIR "${CMAKE_INSTALL_DATADIR}")
    set(LIBRARYDIR "${CMAKE_INSTALL_LIBDIR}")
    set(DOCDIR "${CMAKE_INSTALL_DOCDIR}")

    message(STATUS "prefix: ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "bindir: ${CMAKE_INSTALL_BINDIR}")
    message(STATUS "datadir: ${CMAKE_INSTALL_DATADIR}")
    message(STATUS "docdir: ${CMAKE_INSTALL_DOCDIR}")
    message(STATUS "includedir: ${CMAKE_INSTALL_INCLUDEDIR}")
    message(STATUS "libdir: ${CMAKE_INSTALL_LIBDIR}")
    message(STATUS "cmake: ${CMAKE_VERSION}")
endmacro(ConfigureCMakeVariables)
