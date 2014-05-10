#add_defintions(-D_FC_GUI_ENABLED_)
#add_defintions(-DFREECADMAINPY)

if(FREECAD_BUILD_GUI)
    include_directories(
        ${Boost_INCLUDE_DIRS}
        ${COIN3D_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${SOQT_INCLUDE_DIR}
        ${QT_INCLUDE_DIR}
        ${PYTHON_INCLUDE_PATH}
        ${XERCESC_INCLUDE_DIR}
    )
else(FREECAD_BUILD_GUI)
    include_directories(
        ${Boost_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIR}
        ${QT_INCLUDE_DIR}
        ${PYTHON_INCLUDE_PATH}
        ${XERCESC_INCLUDE_DIR}
    )
endif(FREECAD_BUILD_GUI)

######################## FreeCADMain ########################
if(FREECAD_BUILD_GUI)

    SET(FreeCAD_SRCS
        freecad.rc
        icon.ico
        MainGui.cpp
    )

    if(MSVC)
        SET(FreeCAD_LIBS
            FreeCADGui
            debug qtmaind.lib
            optimized qtmain.lib
        )
    else(MSVC)
        SET(FreeCAD_LIBS
            FreeCADGui
        )
    endif(MSVC)

    add_executable(FreeCADMain WIN32 ${FreeCAD_SRCS})
    target_link_libraries(FreeCADMain ${FreeCAD_LIBS})

    SET_BIN_DIR(FreeCADMain FreeCAD)

    if(WIN32)
        INSTALL(TARGETS FreeCADMain
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    else(WIN32)
        INSTALL(TARGETS FreeCADMain 
            RUNTIME DESTINATION bin
        )
    endif(WIN32)
endif(FREECAD_BUILD_GUI)
######################## FreeCADMainCmd ########################

SET(FreeCADMainCmd_SRCS
    MainCmd.cpp
)
add_executable(FreeCADMainCmd ${FreeCADMainCmd_SRCS})

target_link_libraries(FreeCADMainCmd 
    FreeCADApp
    ${QT_DEBUG_LIBRARIES}
    ${QT_LIBRARIES}
)

SET_BIN_DIR(FreeCADMainCmd FreeCADCmd)

if(WIN32)
    INSTALL(TARGETS FreeCADMainCmd
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS FreeCADMainCmd 
        RUNTIME DESTINATION bin
    )
endif(WIN32)

######################## FreeCADMainPy ########################

SET(FreeCADMainPy_SRCS
     MainPy.cpp
)

add_library(FreeCADMainPy SHARED ${FreeCADMainPy_SRCS})

target_link_libraries(FreeCADMainPy FreeCADApp)

SET_BIN_DIR(FreeCADMainPy FreeCAD)
SET_PYTHON_PREFIX_SUFFIX(FreeCADMainPy)

if(WIN32)
    INSTALL(TARGETS FreeCADMainPy
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS FreeCADMainPy
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(WIN32)

######################## FreeCADGuiPy ########################
if(FREECAD_BUILD_GUI)

    SET(FreeCADGuiPy_SRCS
        FreeCADGuiPy.cpp
    )

    add_library(FreeCADGuiPy SHARED ${FreeCADGuiPy_SRCS})

    target_link_libraries(FreeCADGuiPy FreeCADGui)

    SET_BIN_DIR(FreeCADGuiPy FreeCADGui)
    SET_PYTHON_PREFIX_SUFFIX(FreeCADGuiPy)
    if(WIN32)
        # Name clash with target "FreeCADGui"
        # Must be called "FreeCADGuiPy_d" and "FreeCADGuiPy" to work so override default
        set_target_properties(FreeCADGuiPy PROPERTIES PDB_NAME_DEBUG "FreeCADGuiPy_d")
        set_target_properties(FreeCADGuiPy PROPERTIES PDB_NAME_RELEASE "FreeCADGuiPy")
    endif(WIN32)
    
    if(WIN32)
        INSTALL(TARGETS FreeCADGuiPy
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    else(WIN32)
        INSTALL(TARGETS FreeCADGuiPy
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif(WIN32)
endif(FREECAD_BUILD_GUI)
