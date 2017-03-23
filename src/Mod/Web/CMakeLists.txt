
add_subdirectory(App)
if(BUILD_GUI)
    SET(Web_Scripts
        Webscripts/__init__.py
        Webscripts/Sketchfab.py
    )
    if(Qt5WebKitWidgets_FOUND OR QT_QTWEBKIT_FOUND)
        add_subdirectory(Gui)
    endif()
endif(BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
    DESTINATION
        Mod/Web
)

INSTALL(
    FILES
        Webscripts/__init__.py
        Webscripts/Sketchfab.py
    DESTINATION
        Mod/Web/Webscripts
)
