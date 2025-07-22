
add_subdirectory(App)
add_subdirectory(AppGL)
#if(BUILD_GUI)
#     add_subdirectory(Gui)
#endif(BUILD_GUI)

# install(
    # FILES
        # Gui/PathSimulator.py
    # DESTINATION
        # Mod/CAM
# )
