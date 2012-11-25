include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

set(StartGui_LIBS
    Start
    FreeCADGui
)

qt4_add_resources(Start_QRC_SRCS Resources/Start.qrc)

SET(StartGui_SRCS
    ${Start_QRC_SRCS}
    AppStartGui.cpp
    AppStartGuiPy.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

SET(StartPage_Scripts
    InitGui.py
    StartPage/StartPage.py
    StartPage/__init__.py
)

SET(StartPage_Resources
    StartPage/PartDesign.py
    StartPage/ArchDesign.py
    StartPage/DefaultWorkbench.py
    StartPage/Mesh.py
    StartPage/LoadSchenkel.py
    StartPage/LoadPartDesignExample.py
    StartPage/LoadDrawingExample.py
    StartPage/LoadRobotExample.py
    StartPage/LoadArchExample.py
    StartPage/Background.jpg
    StartPage/FreeCAD.png
    StartPage/ArchDesign.png
    StartPage/PartDesign.png
    StartPage/Mesh.png
    StartPage/Complete.png
    StartPage/LoadMRU0.py
    StartPage/LoadMRU1.py
    StartPage/LoadMRU2.py
    StartPage/PartDesignExample.png
    StartPage/ArchExample.png
    StartPage/web.png
    StartPage/blank.png
    StartPage/complete.jpg
    StartPage/Ship.py
    StartPage/Ship.png
    StartPage/ShipExample.png
)

add_library(StartGui SHARED ${StartGui_SRCS})
target_link_libraries(StartGui ${StartGui_LIBS})

fc_target_copy_resource(StartGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Start
    ${CMAKE_BINARY_DIR}/Mod/Start
    ${StartPage_Scripts})

fc_target_copy_resource(StartGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Start
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Start
    ${StartPage_Resources})

if(MSVC)
    set_target_properties(StartGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(StartGui PROPERTIES DEBUG_OUTPUT_NAME "StartGui_d")
    set_target_properties(StartGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Start)
    set_target_properties(StartGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(StartGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(StartGui PROPERTIES DEBUG_OUTPUT_NAME "StartGui_d")
    set_target_properties(StartGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Start)
    set_target_properties(StartGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(StartGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Start)
    set_target_properties(StartGui PROPERTIES PREFIX "")
    set_target_properties(StartGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS StartGui DESTINATION lib)
