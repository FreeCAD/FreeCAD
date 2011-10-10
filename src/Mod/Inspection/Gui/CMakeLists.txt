include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(InspectionGui_LIBS
    Inspection
    FreeCADGui
)

qt4_add_resources(Inspection_QRC_SRCS Resources/Inspection.qrc)

set(InspectionGui_MOC_HDRS
    VisualInspection.h
)
fc_wrap_cpp(InspectionGui_MOC_SRCS ${InspectionGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${InspectionGui_MOC_SRCS})

set(Dialogs_UIC_SRCS
    VisualInspection.ui
)
qt4_wrap_ui(Dialogs_UIC_HDRS ${Dialogs_UIC_SRCS})
SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    VisualInspection.cpp
    VisualInspection.h
)
SOURCE_GROUP("Dialogs" FILES ${Dialogs_SRCS})

SET(InspectionGui_SRCS
    ${Inspection_QRC_SRCS}
    ${Dialogs_SRCS}
    AppInspectionGui.cpp
    AppInspectionGuiPy.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    ViewProviderInspection.cpp
    ViewProviderInspection.h
    Workbench.cpp
    Workbench.h
)

add_library(InspectionGui SHARED ${InspectionGui_SRCS})
target_link_libraries(InspectionGui ${InspectionGui_LIBS})
fc_copy_script("Mod/Inspection" "InspectionGui" InitGui.py)

if(MSVC)
    set_target_properties(InspectionGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(InspectionGui PROPERTIES DEBUG_OUTPUT_NAME "InspectionGui_d")
    set_target_properties(InspectionGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Inspection)
    set_target_properties(InspectionGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(InspectionGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(InspectionGui PROPERTIES DEBUG_OUTPUT_NAME "InspectionGui_d")
    set_target_properties(InspectionGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Inspection)
    set_target_properties(InspectionGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(InspectionGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Inspection)
    set_target_properties(InspectionGui PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS InspectionGui DESTINATION lib)
