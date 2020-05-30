
add_subdirectory(App)
if(FREECAD_BUILD_GUI)
    #add_subdirectory(Gui)
endif(FREECAD_BUILD_GUI)

INSTALL(
    FILES
        Init.py
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/Mod/Measure
)
