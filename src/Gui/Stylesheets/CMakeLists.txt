
SET(Stylesheets_Files
    "Dark-blue.qss"
    "Dark-green.qss"
    "Dark-orange.qss"
    "Light-blue.qss"
    "Light-green.qss"
    "Light-orange.qss"
)

# Find all the image files
FILE(GLOB Images_Files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/images/*.png")

SOURCE_GROUP("images" FILES ${Images_Files})

ADD_CUSTOM_TARGET(Stylesheets_data ALL
    SOURCES ${Stylesheets_Files} ${Images_Files}
)

fc_copy_sources(Stylesheets_data "${CMAKE_BINARY_DIR}/data/Gui/Stylesheets" ${Stylesheets_Files} ${Images_Files})

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
        ${CMAKE_INSTALL_DATADIR}/Gui/Stylesheets/images
)