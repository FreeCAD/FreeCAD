include(GNUInstallDirs)

if(NOT DEFINED APPDATA_RELEASE_DATE)
    if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
        execute_process(COMMAND git log -1 --pretty=%cd --date=short
            OUTPUT_VARIABLE APPDATA_RELEASE_DATE
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        string(TIMESTAMP APPDATA_RELEASE_DATE "%Y-%m-%d")
    endif()
endif()

configure_file(
    app.parashell.Parashell.metainfo.xml.in
    ${CMAKE_BINARY_DIR}/app.parashell.Parashell.metainfo.xml
)
install(
    FILES ${CMAKE_BINARY_DIR}/app.parashell.Parashell.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
)

install(
    FILES app.parashell.Parashell.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

install(
    FILES app.parashell.Parashell.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mime/packages
)

configure_file(
    Parashell.thumbnailer.in
    ${CMAKE_BINARY_DIR}/Parashell.thumbnailer
)
install(
    FILES ${CMAKE_BINARY_DIR}/Parashell.thumbnailer
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/thumbnailers
)

configure_file(
    ${CMAKE_SOURCE_DIR}/src/Tools/freecad-thumbnailer.in
    ${CMAKE_BINARY_DIR}/freecad-thumbnailer
)
install(
    PROGRAMS ${CMAKE_BINARY_DIR}/freecad-thumbnailer
    TYPE BIN
)
