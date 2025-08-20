SET(PreferencePacks_Files
        "package.xml"
)

SET(PreferencePacks_Directories
        "FreeCAD Classic"
        "Dark behave"
        "FreeCAD Light"
        "FreeCAD Dark"
)

ADD_CUSTOM_TARGET(PreferencePacks_data ALL
        SOURCES ${PreferencePacks_Files} ${PreferencePacks_Directories}
)

FILE(COPY ${PreferencePacks_Files} ${PreferencePacks_Directories} DESTINATION "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Gui/PreferencePacks")

fc_copy_sources(PreferencePacks_data "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Gui/PreferencePacks"
        ${PreferencePacks_Files}
        ${PreferencePacks_Directories})

INSTALL(
        FILES
        ${PreferencePacks_Files}
        DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/PreferencePacks
)

INSTALL(
        DIRECTORY
        ${PreferencePacks_Directories}
        DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/PreferencePacks
)