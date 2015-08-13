
add_subdirectory(Build)
add_subdirectory(3rdParty)
add_subdirectory(Base)
add_subdirectory(App)
add_subdirectory(Main)
add_subdirectory(Mod)
if(BUILD_GUI)
	add_subdirectory(Gui)
	configure_file(Doc/freecad.qhc ${CMAKE_BINARY_DIR}/doc/freecad.qhc COPYONLY)
	configure_file(Doc/freecad.qch ${CMAKE_BINARY_DIR}/doc/freecad.qch COPYONLY)
endif(BUILD_GUI)

if(BUILD_TEMPLATE)
	add_subdirectory(Tools/_TEMPLATE_)
endif(BUILD_TEMPLATE)

add_subdirectory(Doc)

if(FREECAD_MAINTAINERS_BUILD AND WIN32)
	#add_subdirectory(WindowsInstaller)
endif(FREECAD_MAINTAINERS_BUILD AND WIN32)
INSTALL(FILES Doc/freecad.qhc Doc/freecad.qch
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

if(FREECAD_CREATE_MAC_APP)
    add_subdirectory(MacAppBundle)
endif(FREECAD_CREATE_MAC_APP)
