EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c 
"from distutils.sysconfig import get_python_lib; print(get_python_lib())"
OUTPUT_VARIABLE python_libs OUTPUT_STRIP_TRAILING_WHITESPACE )
SET(PYTHON_MAIN_DIR ${python_libs})

set(NAMESPACE_INIT "${CMAKE_BINARY_DIR}/Ext/freecad/__init__.py")
if (WIN32)
	set(FREECAD_LIBRARY_INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
else()
	set(FREECAD_LIBRARY_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
endif()

configure_file(__init__.py.template ${NAMESPACE_INIT})

if (INSTALL_TO_SITEPACKAGES)
    SET(SITE_PACKAGE_DIR ${PYTHON_MAIN_DIR}/freecad)
else ()
    SET(SITE_PACKAGE_DIR Ext/freecad)
endif()

INSTALL(
    FILES
        ${NAMESPACE_INIT}
    DESTINATION
        ${SITE_PACKAGE_DIR}
)
