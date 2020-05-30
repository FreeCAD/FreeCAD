macro(ConfigureCMakeVariables)
    # ================================================================================
    # Output directories for install target

    include(GNUInstallDirs)
    mark_as_advanced(CLEAR
        CMAKE_INSTALL_BINDIR
	CMAKE_INSTALL_DATADIR
	CMAKE_INSTALL_LIBDIR
	CMAKE_INSTALL_INCLUDEDIR
	)

    if(WIN32)
        set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Installation root directory")
    elseif(APPLE)
	# OSX Specific stuff here
    else(WIN32)
	# Assume UNIX and modify installation to freecad subdirectory where appropriate
	set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/freecad)
	set(CMAKE_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR}/freecad)
    endif(WIN32)

    set(PYCXX_INCLUDE_DIR
        "${CMAKE_SOURCE_DIR}/src" CACHE PATH
        "Path to the directory containing PyCXX's CXX/Config.hxx include file")
    set(PYCXX_SOURCE_DIR
        "${CMAKE_SOURCE_DIR}/src/CXX" CACHE PATH
        "Path to the directory containing PyCXX's cxxextensions.c source file")

    # used as compiler defines
    set(RESOURCEDIR "${CMAKE_INSTALL_DATADIR}")
    set(DOCDIR "${CMAKE_INSTALL_DOCDIR}")

    message(STATUS "prefix: ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "bindir: ${CMAKE_INSTALL_BINDIR}")
    message(STATUS "datadir: ${CMAKE_INSTALL_DATADIR}")
    message(STATUS "docdir: ${CMAKE_INSTALL_DOCDIR}")
    message(STATUS "includedir: ${CMAKE_INSTALL_INCLUDEDIR}")
    message(STATUS "libdir: ${CMAKE_INSTALL_LIBDIR}")
    message(STATUS "cmake: ${CMAKE_VERSION}")
endmacro(ConfigureCMakeVariables)
