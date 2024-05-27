
SET(Stylesheets_Files
    "Behave-dark.qss"
    "ProDark.qss"
    "Dark theme.qss"
	"Light theme.qss"
)

SET(Overlay_Stylesheets_Files
    "overlay/Dark Theme + Dark Background.qss"
    "overlay/Dark Theme + Light Background.qss"
    "overlay/Light Theme + Dark Background.qss"
    "overlay/Light Theme + Light Background.qss"
    "overlay/icons/close.svg"
    "overlay/icons/overlay.svg"
    "overlay/icons/float.svg"
    "overlay/icons/autohide.svg"
    "overlay/icons/editshow.svg"
    "overlay/icons/taskshow.svg"
    "overlay/icons/edithide.svg"
    "overlay/icons/mode.svg"
    "overlay/icons/transparent.svg"
    "overlay/icons/taskshow_light.svg"
    "overlay/icons/mode_lighter.svg"
    "overlay/icons/overlay_lighter.svg"
    "overlay/icons/close_lighter.svg"
    "overlay/icons/close_light.svg"
    "overlay/icons/float_lighter.svg"
    "overlay/icons/overlay_light.svg"
    "overlay/icons/mode_light.svg"
    "overlay/icons/transparent_light.svg"
    "overlay/icons/float_light.svg"
    "overlay/icons/transparent_lighter.svg"
    "overlay/icons/close_red.svg"
)

# Find all the image files
FILE(GLOB Images_Files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/images_dark-light/*.svg")

SOURCE_GROUP("images_dark-light" FILES ${Images_Files})

ADD_CUSTOM_TARGET(Stylesheets_data ALL
    SOURCES ${Stylesheets_Files} ${Images_Files} ${Overlay_Stylesheets_Files}
)

fc_copy_sources(Stylesheets_data "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets"
                                  ${Stylesheets_Files}
                                  ${Images_Files}
                                  ${Overlay_Stylesheets_Files})

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
         ${Images_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets/images_dark-light
)
INSTALL(
    FILES
         ${Images_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets/images_classic
)
