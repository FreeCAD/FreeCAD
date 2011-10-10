if(WIN32)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(WIN32)
endif(WIN32)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${SOQT_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${SMSH_INCLUDE_DIR}
    ${SMESH_INCLUDE_DIR}
)

if(MSVC)
    set(CamGui_LIBS
        Cam
        Mesh
        PartGui
        FreeCADGui
        importlib_atlas.lib 
        importlib_umfpackamd.lib
        ${SMSH_LIBRARIES}
        ${SMESH_LIBRARIES}
        #OCCAdaptMesh
    )
else(MSVC)
    set(CamGui_LIBS
        Cam
        PartGui
        FreeCADGui
    )
endif(MSVC)

set(CamGui_MOC_HDRS
    Cutting.h
)
fc_wrap_cpp(CamGui_MOC_SRCS ${CamGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${CamGui_MOC_SRCS})

set(CamGui_UIC_SRCS
    Cutting.ui
)
qt4_wrap_ui(CamGui_UIC_HDRS ${CamGui_UIC_SRCS})

SET(CamGui_SRCS
    ${CamGui_UIC_HDRS}
    AppCamGui.cpp
    Command.cpp
    Cutting.cpp
    Cutting.h
    Cutting.ui
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(CamGui SHARED ${CamGui_SRCS})
target_link_libraries(CamGui ${CamGui_LIBS})
fc_copy_script("Mod/Cam" "CamGui" InitGui.py)

if(MSVC)
    set_target_properties(CamGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(CamGui PROPERTIES DEBUG_OUTPUT_NAME "CamGui_d")
    set_target_properties(CamGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Cam)
    set_target_properties(CamGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(CamGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(CamGui PROPERTIES DEBUG_OUTPUT_NAME "CamGui_d")
    set_target_properties(CamGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Cam)
    set_target_properties(CamGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(CamGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Cam)
    set_target_properties(CamGui PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS CamGui DESTINATION lib)
