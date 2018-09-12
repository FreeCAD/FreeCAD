
SET(StartPage_Scripts
    StartPage.py
    TranslationTexts.py
    __init__.py
)

SET(StartPage_PythonResources
    LoadMRU.py
    LoadExample.py
    LoadNew.py
    LoadCustom.py
    StartPage.css
    StartPage.js
    StartPage.html
    EnableDownload.py
)

SET(StartPage_ImageResources
    images/userhub.png
    images/poweruserhub.png
    images/developerhub.png
    images/manual.png
    images/freecad.png
    images/installed.png
    images/new_file_thumbnail.svg
)

SET(StartPage_Resources
    ${StartPage_PythonResources}
    ${StartPage_ImageResources}
)

add_custom_target(StartPage ALL
    SOURCES ${StartPage_Scripts} ${StartPage_Resources}
)

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

INSTALL(FILES ${StartPage_PythonResources}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Mod/Start/StartPage
)

INSTALL(FILES ${StartPage_ImageResources}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Mod/Start/StartPage/images
)

