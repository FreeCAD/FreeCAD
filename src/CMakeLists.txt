add_subdirectory(Build)
add_subdirectory(3rdParty)
add_subdirectory(Base)
add_subdirectory(App)
add_subdirectory(Main)
add_subdirectory(Mod)
add_subdirectory(Ext)
add_subdirectory(Doc)

configure_file(Doc/ThirdPartyLibraries.html
    ${CMAKE_BINARY_DIR}/doc/ThirdPartyLibraries.html COPYONLY)



if(BUILD_GUI)
    add_subdirectory(Gui)
    configure_file(Doc/freecad.qhc ${CMAKE_BINARY_DIR}/doc/freecad.qhc COPYONLY)
    #configure_file(${CMAKE_BINARY_DIR}/src/Doc/freecad.qch ${CMAKE_BINARY_DIR}/doc/freecad.qch COPYONLY)
	# join offline doc parts
    if(WIN32)
        EXECUTE_PROCESS(COMMAND powershell gc ${CMAKE_SOURCE_DIR}/src/Doc/freecad.qch.part* -Enc Byte -Read 1024 | sc ${CMAKE_BINARY_DIR}/doc/freecad.qch -Enc Byte -Force)
	    # gc is an alias for Get-Content, sc is an alias for Set-Content, 
		#-Read XXXX can be adjusted up for performance, if the user has insufficient RAM it might fail
    else(WIN32)
        EXECUTE_PROCESS(COMMAND sh -c "cat ${CMAKE_SOURCE_DIR}/src/Doc/freecad.qch.part* >> ${CMAKE_BINARY_DIR}/doc/freecad.qch" )
    endif(WIN32)
endif(BUILD_GUI)

if(BUILD_TEMPLATE)
    add_subdirectory(Tools/_TEMPLATE_)
endif(BUILD_TEMPLATE)



if(FREECAD_MAINTAINERS_BUILD AND WIN32)
    #add_subdirectory(WindowsInstaller)
endif(FREECAD_MAINTAINERS_BUILD AND WIN32)

INSTALL(FILES
    Doc/freecad.qhc
    ${CMAKE_BINARY_DIR}/doc/freecad.qch
    Doc/ThirdPartyLibraries.html
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

if(FREECAD_CREATE_MAC_APP)
    add_subdirectory(MacAppBundle)
endif(FREECAD_CREATE_MAC_APP)
