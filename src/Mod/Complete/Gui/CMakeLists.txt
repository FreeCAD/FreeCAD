include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

set(CompleteGui_LIBS
    Complete
    FreeCADGui
)

qt4_add_resources(Complete_QRC_SRCS Resources/Complete.qrc)

SET(CompleteGui_SRCS
    ${Complete_QRC_SRCS}
    AppCompleteGui.cpp
    AppCompleteGuiPy.cpp
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
