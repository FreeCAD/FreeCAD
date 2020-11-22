
SET(Stylesheets_Files
    "Behave-dark.qss"
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

# Find all the image files
FILE(GLOB Images_Files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/images_dark-light/*.svg")

SOURCE_GROUP("images_dark-light" FILES ${Images_Files})

ADD_CUSTOM_TARGET(Stylesheets_data ALL
    SOURCES ${Stylesheets_Files} ${Images_Files}
)

fc_copy_sources(Stylesheets_data "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets"
                                  ${Stylesheets_Files} ${Images_Files})

INSTALL(
    FILES
        ${Stylesheets_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets
)
INSTALL(
    FILES
         ${Images_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets/images_dark-light
)
