EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c
"from distutils.sysconfig import get_python_lib; print(get_python_lib(plat_specific=True))"
OUTPUT_VARIABLE python_libs OUTPUT_STRIP_TRAILING_WHITESPACE )
SET(PYTHON_MAIN_DIR ${python_libs})

set(NAMESPACE_DIR "${CMAKE_BINARY_DIR}/Ext/freecad")
set(NAMESPACE_INIT "${NAMESPACE_DIR}/__init__.py")
if (WIN32)
    get_filename_component(FREECAD_LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}"
                       REALPATH BASE_DIR "${CMAKE_INSTALL_PREFIX}")
    set( ${CMAKE_INSTALL_BINDIR})
else()
    get_filename_component(FREECAD_LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}"
                       REALPATH BASE_DIR "${CMAKE_INSTALL_PREFIX}")
endif()

configure_file(__init__.py.template ${NAMESPACE_INIT})
configure_file(project_utility.py ${NAMESPACE_DIR}/project_utility.py)
configure_file(UiTools.py ${NAMESPACE_DIR}/UiTools.py)

if (INSTALL_TO_SITEPACKAGES)
    SET(SITE_PACKAGE_DIR ${PYTHON_MAIN_DIR}/freecad)
else ()
    SET(SITE_PACKAGE_DIR Ext/freecad)
endif()

INSTALL(
    FILES
        ${NAMESPACE_INIT}
        project_utility.py
        UiTools.py
    DESTINATION
        ${SITE_PACKAGE_DIR}
)
