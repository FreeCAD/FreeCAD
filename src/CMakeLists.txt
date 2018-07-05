add_subdirectory(Build)
add_subdirectory(3rdParty)
add_subdirectory(Base)
add_subdirectory(App)
add_subdirectory(Main)
add_subdirectory(Mod)
add_subdirectory(Ext)
add_subdirectory(Doc)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

if(BUILD_TEMPLATE)
    add_subdirectory(Tools/_TEMPLATE_)
endif(BUILD_TEMPLATE)


if(FREECAD_MAINTAINERS_BUILD AND WIN32)
    #add_subdirectory(WindowsInstaller)
endif(FREECAD_MAINTAINERS_BUILD AND WIN32)

if(FREECAD_CREATE_MAC_APP)
    add_subdirectory(MacAppBundle)
endif(FREECAD_CREATE_MAC_APP)
