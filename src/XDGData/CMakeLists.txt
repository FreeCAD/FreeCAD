include(GNUInstallDirs)

if(NOT DEFINED APPDATA_RELEASE_DATE)
    if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
        execute_process(COMMAND git log -1 --pretty=%cd --date=short
            OUTPUT_VARIABLE APPDATA_RELEASE_DATE
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        if(NOT (CMAKE_VERSION VERSION_LESS 3.8.0))
            string(TIMESTAMP APPDATA_RELEASE_DATE "%Y-%m-%d")
        else()
            file(TIMESTAMP "${CMAKE_SOURCE_DIR}/CMakeLists.txt" APPDATA_RELEASE_DATE "%Y-%m-%d")
        endif()
    endif()
endif()

configure_file(
    org.freecad.FreeCAD.metainfo.xml.in
    ${CMAKE_BINARY_DIR}/org.freecad.FreeCAD.metainfo.xml
)
install(
    FILES ${CMAKE_BINARY_DIR}/org.freecad.FreeCAD.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
)

install(
    FILES org.freecad.FreeCAD.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

install(
    FILES org.freecad.FreeCAD.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mime/packages
)

install(
    FILES FreeCAD.thumbnailer
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/thumbnailers
)
