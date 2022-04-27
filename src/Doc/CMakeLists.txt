
configure_file(ThirdPartyLibraries.html
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DOCDIR}/ThirdPartyLibraries.html COPYONLY)
configure_file(LICENSE.html
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DOCDIR}/LICENSE.html COPYONLY)

# The CONTRIBUTORS file is now compiled into the FreeCAD GUI Qt resource file

INSTALL(FILES
    ThirdPartyLibraries.html
    LICENSE.html
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

if(DOXYGEN_FOUND)

    IF (DOXYGEN_DOT_EXECUTABLE)
        SET(HAVE_DOT YES)
    ELSE (DOXYGEN_DOT_EXECUTABLE)
        message(STATUS "Note: Doxygen docs will look better with graphviz's dot installed.")
        SET(HAVE_DOT NO)
    ENDIF (DOXYGEN_DOT_EXECUTABLE)


    SET(DOXYGEN_LANGUAGE "English" CACHE STRING "Language used by doxygen")
    MARK_AS_ADVANCED(DOXYGEN_LANGUAGE)

# note: this test is obsolete if DevDoc target is used
    if (NOT BUILD_GUI)
        message("Note: Gui is not built. Documentation may lack some parts.")
    endif (NOT BUILD_GUI)

    # directory order seems important for correct macro expansion
    # (files containing macros definitions must be parsed before the files using them)
    SET(DOXYGEN_SOURCE_DIR ${COIN3D_INCLUDE_DIRS}/Inventor/fields/SoSubField.h 
                           ${CMAKE_SOURCE_DIR}/src/CXX 
                           ${CMAKE_SOURCE_DIR}/src/zipios++ 
                           ${CMAKE_SOURCE_DIR}/src/3rdParty
                           ${CMAKE_SOURCE_DIR}/src/Build 
                           ${CMAKE_SOURCE_DIR}/src/Base 
                           ${CMAKE_BINARY_DIR}/src/Base 
                           ${CMAKE_SOURCE_DIR}/src/App 
                           ${CMAKE_BINARY_DIR}/src/App 
                           ${CMAKE_SOURCE_DIR}/src/Gui 
                           ${CMAKE_BINARY_DIR}/src/Gui 
                           ${CMAKE_SOURCE_DIR}/src/Mod
                           ${CMAKE_BINARY_DIR}/src/Mod
                           #${CMAKE_SOURCE_DIR}/src/Main #nothing to document there ATM...
                           ${CMAKE_SOURCE_DIR}/src/Doc
                           ${CMAKE_BINARY_DIR}/src/Doc 
    )
    STRING(REGEX REPLACE ";" " " DOXYGEN_INPUT_LIST "${DOXYGEN_SOURCE_DIR}")

    # exclude some subdirectories
    # src/Tools : content not relevant ?
    set(        DOXYGEN_EXCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/Tools)
    list(APPEND DOXYGEN_EXCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/Doc/sphinx)
    # deprecated modules
    list(APPEND DOXYGEN_EXCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/Mod/Complete)

    STRING(REGEX REPLACE ";" " " DOXYGEN_EXCLUDE_LIST "${DOXYGEN_EXCLUDE_DIR}")

    SET(DOXYGEN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/doc/SourceDocu)
    SET(DOXYGEN_IMAGE_PATH ${CMAKE_SOURCE_DIR}/src/Gui/Icons)
    SET(DOXYGEN_LAYOUT_FILE ${CMAKE_SOURCE_DIR}/src/Doc/FreecadDoxygenLayout.xml)

    find_package(Coin3DDoc)
    if( COIN3D_DOC_FOUND )
        SET(DOXYGEN_TAGFILES 
            ${COIN3D_DOC_TAGFILE}=${COIN3D_DOC_PATH}
        )
    endif( COIN3D_DOC_FOUND )

    # Try to get some git information about the source tree
    EXECUTE_PROCESS(COMMAND git rev-parse --abbrev-ref HEAD
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    OUTPUT_VARIABLE GIT_DOC_REV
                    ERROR_VARIABLE GIT_REV_FAILED
                    RESULT_VARIABLE GIT_REV_RETURN_CODE)
    if( GIT_REV_RETURN_CODE EQUAL 0 )
        EXECUTE_PROCESS(COMMAND git log -n1 "--pretty=format:%h %cD '%s'"
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                        OUTPUT_VARIABLE GIT_DOC_LOG
                        ERROR_VARIABLE GIT_REV_FAILED
                        RESULT_VARIABLE GIT_REV_RETURN_CODE)
        set(GIT_REVISION_INFO
            "Built from git branch ${GIT_DOC_REV}, commit ${GIT_DOC_LOG}")
    endif()

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mainpage.dox.in
                   ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/BuildDevDoc.cfg.in  
                   ${CMAKE_CURRENT_BINARY_DIR}/BuildDevDoc.cfg @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/BuildWebDoc.cfg.in  
                   ${CMAKE_CURRENT_BINARY_DIR}/BuildWebDoc.cfg @ONLY)
 
    # reconfigure to have all sources files generated from xml, whatever build options were
    # generate source documentation
    # delete CMakeCache to reset to default build options next time 
    ADD_CUSTOM_TARGET(DevDoc
        ${CMAKE_COMMAND} -D BUILD_GUI:BOOL=ON -D BUILD_FEM:BOOL=ON ${CMAKE_SOURCE_DIR}
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/BuildDevDoc.cfg
        COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    # lightweight version for the web
    ADD_CUSTOM_TARGET(WebDoc
        ${CMAKE_COMMAND} -D BUILD_GUI:BOOL=ON -D BUILD_FEM:BOOL=ON ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/Doc/templates ${CMAKE_CURRENT_BINARY_DIR}/templates
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/BuildWebDoc.cfg
        COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

else( DOXYGEN_FOUND )
    ADD_CUSTOM_TARGET(DevDoc
                      echo "Doxygen not found.  Install it and rerun CMake")
endif( DOXYGEN_FOUND )
