EXECUTE_PROCESS(COMMAND ${Python3_EXECUTABLE} -c
    "from sysconfig import get_path; print(get_path('purelib'))"
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
