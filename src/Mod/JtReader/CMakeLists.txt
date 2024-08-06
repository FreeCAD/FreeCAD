add_subdirectory(App)
if(BUILD_GUI)
    # add_subdirectory(Gui)
endif(BUILD_GUI)

install(FILES Init.py DESTINATION Mod/JtReader)
