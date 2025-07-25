
SET(Stylesheets_Files
	"FreeCAD.qss"
	#remove below after testing new stylesheet system 8/6/2025
	"FreeCAD Dark.qss"
	"FreeCAD Light.qss"
)

SET(Overlay_Stylesheets_Files
    "overlay/Freecad Overlay.qss"
	#remove below after testing new stylesheet system 8/6/2025
	"overlay/Dark Theme + Dark Background.qss"
    "overlay/Dark Theme + Light Background.qss"
    "overlay/Light Theme + Dark Background.qss"
    "overlay/Light Theme + Light Background.qss"
)

# Find all the image files
FILE(GLOB Images_Files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/images_dark-light/*.svg")

SOURCE_GROUP("Images_dark-light" FILES ${Images_Files})

FILE(GLOB Images_Files2 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/images_classic/*.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/images_classic/*.png"
)

SOURCE_GROUP("images_classic" FILES ${Images_Files2})

FILE(GLOB Overlay_icons RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/overlay/icons/*.svg")

SOURCE_GROUP("overlay_icons" FILES ${Overlay_icons})


ADD_CUSTOM_TARGET(Stylesheets_data ALL
    SOURCES ${Stylesheets_Files} ${Images_Files} ${Images_Files2} ${Overlay_Stylesheets_Files} ${Overlay_icons}
)

fc_copy_sources(Stylesheets_data "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets"
                                  ${Stylesheets_Files}
                                  ${Images_Files}
                                  ${Images_Files2}
                                  ${Overlay_Stylesheets_Files}
                                  ${Overlay_icons})

INSTALL(
    FILES
        ${Stylesheets_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets
)
INSTALL(
    FILES
        ${Overlay_Stylesheets_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets/overlay
)
INSTALL(
    FILES
        ${Overlay_icons}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets/overlay/icons
)
INSTALL(
    FILES
         ${Images_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets/images_dark-light
)
INSTALL(
    FILES
         ${Images_Files2}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets/images_classic
)
