if (${PYTHON_VERSION_STRING} VERSION_LESS "3.10")
    # deprecated distutils package still required for ubuntu 20.04 and similar
    # see https://bugs.launchpad.net/ubuntu/+source/python3.8/+bug/2039511
    # TODO: remove this once we drop support for 20.04
    EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c
        "from distutils.sysconfig import get_python_lib; print(get_python_lib(plat_specific=True))"
        OUTPUT_VARIABLE python_libs OUTPUT_STRIP_TRAILING_WHITESPACE )
else()
    EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c
        "from sysconfig import get_path; print(get_path('purelib'))"
        OUTPUT_VARIABLE python_libs OUTPUT_STRIP_TRAILING_WHITESPACE )
endif()

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

set(EXT_FILES
    freecad_doc.py
    module_io.py
    part.py
    partdesign.py
    project_utility.py
    sketcher.py
    UiTools.py
    utils.py
    utils_zip.py
)

foreach (it ${EXT_FILES})
    configure_file(${it} ${NAMESPACE_DIR}/${it})
endforeach()

if (INSTALL_TO_SITEPACKAGES)
    SET(SITE_PACKAGE_DIR ${PYTHON_MAIN_DIR}/freecad)
else ()
    SET(SITE_PACKAGE_DIR Ext/freecad)
endif()

INSTALL(
    FILES
        ${NAMESPACE_INIT}
        ${EXT_FILES}
    DESTINATION
        ${SITE_PACKAGE_DIR}
)
