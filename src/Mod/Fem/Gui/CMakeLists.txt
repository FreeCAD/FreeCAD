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

qt4_add_resources(FemResource_SRCS Resources/Fem.qrc)

SOURCE_GROUP("Resources" FILES ${FemResource_SRCS})


SET(FemGui_SRCS
    #${FemGui_UIC_HDRS}
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
fc_copy_script("Mod/Fem" "FemGui" InitGui.py)

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
endif(MSVC)

install(TARGETS FemGui DESTINATION lib)
