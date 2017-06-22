SET(CompleteGuiIcon_SVG
    Resources/icons/CompleteWorkbench.svg
)

SOURCE_GROUP("" FILES ${CompleteGuiIcon_SVG})

ADD_CUSTOM_TARGET(CompleteGui ALL
    SOURCES ${CompleteGuiIcon_SVG}
)

fc_copy_sources(CompleteGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Complete" ${CompleteGuiIcon_SVG})

INSTALL(FILES ${CompleteGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Complete/Resources/icons")

