add_subdirectory(App)

set(Start_Scripts
    Init.py
)

if(BUILD_GUI)
    SET(StartPage_Scripts
        StartPage.py
        TranslationTexts.py
        __init__.py
    )

    SET(StartPage_Resources
        LoadMRU.py
        LoadExample.py
        LoadNew.py
        LoadCustom.py
        StartPage.css
        StartPage.js
        StartPage.html
        EnableDownload.py
        images/userhub.png
        images/poweruserhub.png
        images/developerhub.png
        images/manual.png
        images/freecad.png
        images/installed.png
    )

    list(APPEND Start_Scripts InitGui.py)

    add_subdirectory(Gui)
    add_subdirectory(StartPage)
endif(BUILD_GUI)

add_custom_target(StartScripts ALL
    SOURCES ${Start_Scripts}
)

fc_target_copy_resource(StartScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Start
    ${Start_Scripts})

INSTALL(
    FILES
        ${Start_Scripts}
    DESTINATION
        Mod/Start
)
