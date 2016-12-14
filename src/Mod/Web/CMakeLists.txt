
add_subdirectory(App)
if(BUILD_GUI)
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
