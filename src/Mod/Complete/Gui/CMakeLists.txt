include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

set(CompleteGui_LIBS
    Complete
    FreeCADGui
)

qt4_add_resources(Complete_QRC_SRCS Resources/Complete.qrc)

SET(CompleteGui_SRCS
    ${Complete_QRC_SRCS}
    AppCompleteGui.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(CompleteGui SHARED ${CompleteGui_SRCS})
target_link_libraries(CompleteGui ${CompleteGui_LIBS})


fc_target_copy_resource(CompleteGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Complete
    ${CMAKE_BINARY_DIR}/Mod/Complete
    InitGui.py)

SET_BIN_DIR(CompleteGui CompleteGui /Mod/Complete)
SET_PYTHON_PREFIX_SUFFIX(CompleteGui)

install(TARGETS CompleteGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

SET(CompleteGuiIcon_SVG
    Resources/icons/CompleteWorkbench.svg
)

fc_copy_sources(CompleteGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Complete" ${CompleteGuiIcon_SVG})

INSTALL(FILES ${CompleteGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Complete/Resources/icons")
