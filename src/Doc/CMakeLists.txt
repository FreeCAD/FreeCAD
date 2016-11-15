find_package(Doxygen)
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
                           ${CMAKE_SOURCE_DIR}/src/Main 
                           ${CMAKE_SOURCE_DIR}/src/Doc 
    )
    STRING(REGEX REPLACE ";" " " DOXYGEN_INPUT_LIST "${DOXYGEN_SOURCE_DIR}")

    # exclude some subdirectories
    # src/Tools : content not relevant ?
    # src/3rdParty/CxImage : Windows thumbnailer
    set(        DOXYGEN_EXCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/Tools)
    list(APPEND DOXYGEN_EXCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/3rdParty/CxImage)
    list(APPEND DOXYGEN_EXCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/3rdParty/salomesmesh)
    list(APPEND DOXYGEN_EXCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/Mod/Assembly)

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

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/BuildDevDoc.cfg.in  
                   ${CMAKE_CURRENT_BINARY_DIR}/BuildDevDoc.cfg @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/BuildWebDoc.cfg.in  
                   ${CMAKE_CURRENT_BINARY_DIR}/BuildWebDoc.cfg @ONLY)
                
    if( FREECAD_MAINTAINERS_BUILD )
        if (WIN32)
        ADD_CUSTOM_TARGET(SourceDocu 
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/BuildDevDoc.cfg 
            COMMAND ${QT_HELPCOMPILER_EXECUTABLE} "\"${CMAKE_BINARY_DIR}/doc/SourceDocu/html/index.qhp\""
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/doc/SourceDocu/html/index.qch ${CMAKE_BINARY_DIR}/doc/FreeCADSource.qch
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/BuildDevDoc.cfg
        )
        else(WIN32)
        ADD_CUSTOM_TARGET(SourceDocu 
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/BuildDevDoc.cfg 
            COMMAND ${QT_HELPCOMPILER_EXECUTABLE} "${CMAKE_BINARY_DIR}/doc/SourceDocu/html/index.qhp"
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/doc/SourceDocu/html/index.qch ${CMAKE_BINARY_DIR}/doc/FreeCADSource.qch
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/BuildDevDoc.cfg
        )
        endif(WIN32)

        ADD_CUSTOM_TARGET(UserDocu 
            ${PYTHON_EXECUTABLE} 
            ${CMAKE_SOURCE_DIR}/src/Tools/wiki2qhelp.py 
         -c ${QT_HELPCOMPILER_EXECUTABLE}   
         -g ${QT_COLLECTIOMGENERATOR_EXECUTABLE}   
         -o ${CMAKE_BINARY_DIR}/doc   
        )

    else( FREECAD_MAINTAINERS_BUILD )
 
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

    endif( FREECAD_MAINTAINERS_BUILD )
else( DOXYGEN_FOUND )
    ADD_CUSTOM_TARGET(DevDoc
                      echo "Doxygen not found.  Install it and rerun CMake")
endif( DOXYGEN_FOUND )

