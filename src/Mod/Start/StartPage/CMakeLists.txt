
INSTALL(FILES ${StartPage_Scripts}
    DESTINATION Mod/Start/StartPage
)

INSTALL(FILES ${StartPage_Resources}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Mod/Start/StartPage
)

INSTALL(DIRECTORY images/ 
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Mod/Start/StartPage/images
)
