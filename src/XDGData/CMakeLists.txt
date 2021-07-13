set(APPDATA_RELEASE_VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${FREECAD_VERSION_PATCH}${PACKAGE_VERSION_SUFFIX}")

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
    org.freecadweb.FreeCAD.appdata.xml.in
    org.freecadweb.FreeCAD.appdata.xml
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freecadweb.FreeCAD.appdata.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
)

install(
    FILES org.freecadweb.FreeCAD.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

install(
    FILES org.freecadweb.FreeCAD.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mime/packages
)

install(
    FILES FreeCAD.thumbnailer
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/thumbnailers
)

install(
    FILES ../Gui/Icons/freecad-icon-16.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps
    RENAME org.freecadweb.FreeCAD.png
)

install(
    FILES ../Gui/Icons/freecad-icon-32.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps
    RENAME org.freecadweb.FreeCAD.png
)

install(
    FILES ../Gui/Icons/freecad-icon-48.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps
    RENAME org.freecadweb.FreeCAD.png
)

install(
    FILES ../Gui/Icons/freecad-icon-64.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps
    RENAME org.freecadweb.FreeCAD.png
)

install(
    FILES ../Gui/Icons/freecad.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    RENAME org.freecadweb.FreeCAD.svg
)

install(
    FILES ../Gui/Icons/freecad.xpm
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps
    RENAME org.freecadweb.FreeCAD.xpm
)

install(
    FILES ../Gui/Icons/freecad-doc.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/mimetypes
    RENAME org.freecadweb.FreeCAD-mimetype.svg
)

install(
    FILES ../Gui/Icons/freecad-doc-64.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/mimetypes
    RENAME org.freecadweb.FreeCAD-mimetype.png
)
