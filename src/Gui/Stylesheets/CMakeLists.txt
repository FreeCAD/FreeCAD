
SET(Stylesheets_Files
    "Dark-blue.qss"
    "Dark-green.qss"
    "Dark-orange.qss"
    "Darker-blue.qss"
    "Darker-green.qss"
    "Darker-orange.qss"
    "Light-blue.qss"
    "Light-green.qss"
    "Light-orange.qss"
    "Dark-contrast.qss"
)

SET(Overlay_Stylesheets_Files
    "overlay/Dark.qss"
    "overlay/Light.qss"
    "overlay/Dark-Outline.qss"
    "overlay/Light-Outline.qss"
    # "overlay/Dark-off.qss"
    # "overlay/Light-off.qss"
    "overlay/close.svg"
    "overlay/overlay.svg"
    "overlay/float.svg"
    "overlay/autohide.svg"
    "overlay/editshow.svg"
    "overlay/taskshow.svg"
    "overlay/edithide.svg"
    "overlay/mode.svg"
    "overlay/transparent.svg"
)

SET(Menu_Stylesheets_Files
    "menu/Default.qss"
    "menu/Dark.qss"
    "menu/Light.qss"
)

# Find all the image files
FILE(GLOB Images_Files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/images_dark-light/*.svg")

SOURCE_GROUP("images_dark-light" FILES ${Images_Files})

ADD_CUSTOM_TARGET(Stylesheets_data ALL
    SOURCES ${Stylesheets_Files} ${Images_Files}
    ${Overlay_Stylesheets_Files} ${Menu_Stylesheets_Files}
)

fc_copy_sources(Stylesheets_data "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets"
                                  ${Stylesheets_Files} 
                                  ${Images_Files} 
                                  ${Overlay_Stylesheets_Files}
                                  ${Menu_Stylesheets_Files})

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
        ${Menu_Stylesheets_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets/menu
)
INSTALL(
    FILES
         ${Images_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets/images_dark-light
)
