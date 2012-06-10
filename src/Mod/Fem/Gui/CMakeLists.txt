if(MSVC)
    add_definitions(-DFCGuiFem -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(FemGui_LIBS
    Fem
    FreeCADGui
)

if(SMESH_FOUND)
    include_directories( ${SMESH_INCLUDE_DIR} )
    list( APPEND FemGui_LIBS ${SMESH_LIBRARIES} )
endif(SMESH_FOUND)

set(FemGui_MOC_HDRS
    Hypothesis.h
)
fc_wrap_cpp(FemGui_MOC_SRCS ${FemGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${FemGui_MOC_SRCS})

set(FemGui_UIC_SRCS
    Hypothesis.ui
)
qt4_wrap_ui(FemGui_UIC_HDRS ${FemGui_UIC_SRCS})
SET(FemGui_DLG_SRCS
    ${FemGui_UIC_HDRS}
    Hypothesis.ui
    Hypothesis.cpp
    Hypothesis.h
)
SOURCE_GROUP("Dialogs" FILES ${FemGui_DLG_SRCS})

qt4_add_resources(FemResource_SRCS Resources/Fem.qrc)

SOURCE_GROUP("Resources" FILES ${FemResource_SRCS})


SET(FemGui_SRCS
    ${FemGui_DLG_SRCS}
    ${FemResource_SRCS}
    AppFemGui.cpp
    AppFemGuiPy.cpp
    Command.cpp
    Resources/Fem.qrc
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
    ViewProviderFemMesh.cpp
    ViewProviderFemMesh.h
)

add_library(FemGui SHARED ${FemGui_SRCS})
target_link_libraries(FemGui ${FemGui_LIBS})


fc_target_copy_resource(FemGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Fem
    ${CMAKE_BINARY_DIR}/Mod/Fem
    InitGui.py)

if(MSVC)
    set_target_properties(FemGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(FemGui PROPERTIES DEBUG_OUTPUT_NAME "FemGui_d")
    set_target_properties(FemGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Fem)
    set_target_properties(FemGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(FemGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(FemGui PROPERTIES DEBUG_OUTPUT_NAME "FemGui_d")
    set_target_properties(FemGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Fem)
    set_target_properties(FemGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(FemGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Fem)
    set_target_properties(FemGui PROPERTIES PREFIX "")
    set_target_properties(FemGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS FemGui DESTINATION lib)
