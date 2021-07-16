
SET(PreferencePackBehaviorTemplates_Files
  Main_window_layout.cfg
)

ADD_CUSTOM_TARGET(PreferencePackBehaviorTemplates_data ALL
    SOURCES ${PreferencePackBehaviorTemplates_Files}
)

fc_copy_sources(PreferencePackBehaviorTemplates_data "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Gui/PreferencePackTemplates/Behavior"
                                  ${PreferencePackBehaviorTemplates_Files})

INSTALL(
    FILES
        ${PreferencePackBehaviorTemplates_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/PreferencePackTemplates/Behavior
)