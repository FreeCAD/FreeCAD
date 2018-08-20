
add_custom_target(StartPage ALL
    SOURCES ${StartPage_Scripts} ${StartPage_Resources}
)

fc_target_copy_resource(StartPage
    ${CMAKE_SOURCE_DIR}/src/Mod/Start
    ${CMAKE_BINARY_DIR}/Mod/Start
    ${Start_Scripts})

fc_target_copy_resource(StartPage
    ${CMAKE_SOURCE_DIR}/src/Mod/Start/StartPage
    ${CMAKE_BINARY_DIR}/Mod/Start/StartPage
    ${StartPage_Scripts})

fc_target_copy_resource(StartPage
    ${CMAKE_SOURCE_DIR}/src/Mod/Start/StartPage
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Start/StartPage
    ${StartPage_Resources})

INSTALL(FILES ${StartPage_Scripts}
    DESTINATION Mod/Start/StartPage
)

INSTALL(FILES ${StartPage_Resources}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Mod/Start/StartPage
)

