
SET(PreferencePackTemplates_Files
    Arch_Colors.cfg
    Console_Colors.cfg
    Draft_Colors.cfg
    Editor_Colors.cfg
    Editor_Font.cfg
    Main_window_layout.cfg
    Part_Colors.cfg
    Path_Colors.cfg
    Shortcuts.cfg
    Sketcher_Colors.cfg
    Start_Colors.cfg
    TechDraw_Colors.cfg
    View.cfg
    Window_Colors.cfg
)

ADD_CUSTOM_TARGET(PreferencePackTemplates_data ALL
    SOURCES ${PreferencePackTemplates_Files}
)

fc_copy_sources(PreferencePackTemplates_data "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Gui/PreferencePackTemplates"
                                  ${PreferencePackTemplates_Files})

INSTALL(
    FILES
        ${PreferencePackTemplates_Files}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Gui/PreferencePackTemplates
)
