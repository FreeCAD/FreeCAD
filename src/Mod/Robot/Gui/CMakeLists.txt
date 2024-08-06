include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS})
link_directories(${OCC_LIBRARY_DIR})

set(RobotGui_LIBS Robot FreeCADGui)

set(Robot_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Robot_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
                            ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Robot_TR_QRC} ${QM_SRCS})
qt_add_resources(RobotResource_SRCS Resources/Robot.qrc ${Robot_TR_QRC})
source_group("Resources" FILES ${RobotResource_SRCS})

set(RobotGui_UIC_SRCS
    TaskTrajectory.ui
    TaskRobot6Axis.ui
    TrajectorySimulate.ui
    TaskRobotMessages.ui
    TaskRobotControl.ui
    TaskEdge2TracParameter.ui
    TaskTrajectoryDressUpParameter.ui)

set(RobotGui_SRCS_Module AppRobotGui.cpp Resources/Robot.qrc PreCompiled.cpp PreCompiled.h
                         Workbench.cpp Workbench.h)
source_group("Module" FILES ${RobotGui_SRCS_Module})

set(RobotGui_SRCS_Commands Command.cpp CommandTrajectory.cpp CommandInsertRobot.cpp
                           CommandExport.cpp)
source_group("Commands" FILES ${RobotGui_SRCS_Commands})

set(RobotGui_SRCS_ViewProvider
    ViewProviderRobotObject.cpp
    ViewProviderRobotObject.h
    ViewProviderTrajectory.cpp
    ViewProviderTrajectory.h
    ViewProviderEdge2TracObject.cpp
    ViewProviderEdge2TracObject.h
    ViewProviderTrajectoryCompound.cpp
    ViewProviderTrajectoryCompound.h
    ViewProviderTrajectoryDressUp.cpp
    ViewProviderTrajectoryDressUp.h)
source_group("ViewProvider" FILES ${RobotGui_SRCS_ViewProvider})

set(RobotGui_SRCS_TaskBoxes
    TaskRobot6Axis.ui
    TaskRobot6Axis.cpp
    TaskRobot6Axis.h
    TaskTrajectory.ui
    TaskTrajectory.cpp
    TaskTrajectory.h
    TaskRobotMessages.ui
    TaskRobotMessages.h
    TaskRobotMessages.cpp
    TaskRobotControl.ui
    TaskRobotControl.h
    TaskRobotControl.cpp
    TrajectorySimulate.ui
    TrajectorySimulate.cpp
    TrajectorySimulate.h
    TaskEdge2TracParameter.ui
    TaskEdge2TracParameter.cpp
    TaskEdge2TracParameter.h
    TaskTrajectoryDressUpParameter.ui
    TaskTrajectoryDressUpParameter.cpp
    TaskTrajectoryDressUpParameter.h
    TaskWatcher.h
    TaskWatcher.cpp)
source_group("Task_Boxes" FILES ${RobotGui_SRCS_TaskBoxes})

set(RobotGui_SRCS_TaskDlg
    TaskDlgSimulate.h
    TaskDlgSimulate.cpp
    TaskDlgEdge2Trac.h
    TaskDlgEdge2Trac.cpp
    TaskDlgTrajectoryCompound.h
    TaskDlgTrajectoryCompound.cpp
    TaskDlgTrajectoryDressUp.h
    TaskDlgTrajectoryDressUp.cpp)
source_group("Task_Dialogs" FILES ${RobotGui_SRCS_TaskDlg})

set(RobotGui_SRCS
    ${RobotGui_UIC_HDRS}
    ${RobotResource_SRCS}
    ${RobotGui_SRCS_Module}
    ${RobotGui_SRCS_ViewProvider}
    ${RobotGui_SRCS_Commands}
    ${RobotGui_SRCS_TaskBoxes}
    ${RobotGui_SRCS_TaskDlg})

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    get_msvc_precompiled_source("PreCompiled.cpp" PCH_SRCS ${RobotGui_SRCS})
    add_msvc_precompiled_header(RobotGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

set(RobotGuiIcon_SVG Resources/icons/RobotWorkbench.svg)

add_library(RobotGui SHARED ${RobotGui_SRCS} ${RobotGuiIcon_SVG})
target_link_libraries(RobotGui ${RobotGui_LIBS})
if(FREECAD_WARN_ERROR)
    target_compile_warn_error(RobotGui)
endif()

unset(_flag_found CACHE)
check_cxx_compiler_flag("-Wno-deprecated-copy" _flag_found)
if(_flag_found)
    target_compile_options(RobotGui PRIVATE -Wno-deprecated-copy)
endif()

set_bin_dir(RobotGui RobotGui /Mod/Robot)
set_python_prefix_suffix(RobotGui)

fc_copy_sources(RobotGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Robot"
                ${RobotGuiIcon_SVG})

install(TARGETS RobotGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${RobotGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Robot/Resources/icons")
