
add_subdirectory(App)
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
    )

    add_subdirectory(Gui)
    add_subdirectory(StartPage)
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
    DESTINATION
        Mod/Start
)
