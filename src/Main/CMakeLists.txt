#add_defintions(-D_FC_GUI_ENABLED_)
#add_defintions(-DFREECADMAINPY)

configure_file(freecad.rc.cmake ${CMAKE_CURRENT_BINARY_DIR}/freecad.rc)
configure_file(freecadCmd.rc.cmake ${CMAKE_CURRENT_BINARY_DIR}/freecadCmd.rc)
file(COPY icon.ico DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if(BUILD_GUI)
    include_directories(
        ${Boost_INCLUDE_DIRS}
        ${COIN3D_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIR}
        ${QT_INCLUDE_DIR}
        ${PYTHON_INCLUDE_DIRS}
        ${XercesC_INCLUDE_DIRS}
    )
else(BUILD_GUI)
    include_directories(
        ${Boost_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIR}
        ${QT_INCLUDE_DIR}
        ${PYTHON_INCLUDE_DIRS}
        ${XercesC_INCLUDE_DIRS}
    )
endif(BUILD_GUI)

######################## FreeCADMain ########################
if(BUILD_GUI)

    SET(FreeCAD_SRCS
        ${CMAKE_CURRENT_BINARY_DIR}/freecad.rc
        icon.ico
        MainGui.cpp
    )

    SET(FreeCAD_LIBS
        FreeCADGui
    )

    if(NOT BUILD_DYNAMIC_LINK_PYTHON)
        # executables have to be linked against python libraries,
        # because extension modules are not.
        list(APPEND FreeCAD_LIBS
            ${PYTHON_LIBRARIES}
        )
    endif(NOT BUILD_DYNAMIC_LINK_PYTHON)

    add_executable(FreeCADMain WIN32 ${FreeCAD_SRCS})
    target_link_libraries(FreeCADMain ${FreeCAD_LIBS})

    SET_BIN_DIR(FreeCADMain FreeCAD)

    if(WIN32)
        INSTALL(TARGETS FreeCADMain
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    elseif(APPLE AND NOT BUILD_WITH_CONDA)
        INSTALL(TARGETS FreeCADMain
        RUNTIME DESTINATION MacOS
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    else()
        INSTALL(TARGETS FreeCADMain
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endif(BUILD_GUI)
######################## FreeCADMainCmd ########################

SET(FreeCADMainCmd_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/freecadCmd.rc
    icon.ico
    MainCmd.cpp
)

add_executable(FreeCADMainCmd ${FreeCADMainCmd_SRCS})

SET(FreeCADMainCmd_LIBS
    FreeCADApp
    ${QtCore_LIBRARIES}
    ${QtXml_LIBRARIES}
)

if(NOT BUILD_DYNAMIC_LINK_PYTHON)
    # executables have to be linked against python libraries,
    # because extension modules are not.
    list(APPEND FreeCADMainCmd_LIBS
        ${PYTHON_LIBRARIES}
    )
endif(NOT BUILD_DYNAMIC_LINK_PYTHON)

target_link_libraries(FreeCADMainCmd
    ${FreeCADMainCmd_LIBS}
)

SET_BIN_DIR(FreeCADMainCmd FreeCADCmd)

if(WIN32)
    INSTALL(TARGETS FreeCADMainCmd
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
elseif(APPLE AND NOT BUILD_WITH_CONDA)
    INSTALL(TARGETS FreeCADMainCmd
        RUNTIME DESTINATION MacOS
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else()
    INSTALL(TARGETS FreeCADMainCmd
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

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
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS FreeCADMainPy
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(WIN32)

######################## FreeCADGuiPy ########################
if(BUILD_GUI)

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
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    else(WIN32)
        INSTALL(TARGETS FreeCADGuiPy
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif(WIN32)
endif(BUILD_GUI)
