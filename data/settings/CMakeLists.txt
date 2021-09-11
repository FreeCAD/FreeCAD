
SET(Conf_Files 
    LargeFontIcon.FCParam
    VerticalToolBars.FCParam
    VerticalSplit.FCParam
    OverlayDark.FCParam
    OverlayLight.FCParam
    DisplaySettings.FCParam
)

ADD_CUSTOM_TARGET(Conf_data ALL
    SOURCES ${Conf_Files}
)

if(NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    fc_copy_sources(Conf_data "${CMAKE_BINARY_DIR}/share/settings" ${Conf_Files})
endif()

INSTALL(
    FILES
        ${Conf_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/settings
)

