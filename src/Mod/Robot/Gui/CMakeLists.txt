# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

set(RobotGui_LIBS
    Robot
    FreeCADGui
)

set (Robot_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Robot_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Robot_TR_QRC} ${QM_SRCS})
qt_add_resources(RobotResource_SRCS Resources/Robot.qrc ${Robot_TR_QRC})
SOURCE_GROUP("Resources" FILES ${RobotResource_SRCS})

set(RobotGui_UIC_SRCS
    TaskTrajectory.ui
    TaskRobot6Axis.ui
    TrajectorySimulate.ui
    TaskRobotMessages.ui
    TaskRobotControl.ui
    TaskEdge2TracParameter.ui
    TaskTrajectoryDressUpParameter.ui
)

SET(RobotGui_SRCS_Module
    AppRobotGui.cpp
    Resources/Robot.qrc
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)
SOURCE_GROUP("Module" FILES ${RobotGui_SRCS_Module})

SET(RobotGui_SRCS_Commands
    Command.cpp
    CommandTrajectory.cpp
    CommandInsertRobot.cpp
    CommandExport.cpp
)
SOURCE_GROUP("Commands" FILES ${RobotGui_SRCS_Commands})

SET(RobotGui_SRCS_ViewProvider
    ViewProviderRobotObject.cpp
    ViewProviderRobotObject.h
    ViewProviderTrajectory.cpp
    ViewProviderTrajectory.h
    ViewProviderEdge2TracObject.cpp
    ViewProviderEdge2TracObject.h
    ViewProviderTrajectoryCompound.cpp
    ViewProviderTrajectoryCompound.h
    ViewProviderTrajectoryDressUp.cpp
    ViewProviderTrajectoryDressUp.h
)
SOURCE_GROUP("ViewProvider" FILES ${RobotGui_SRCS_ViewProvider})

SET(RobotGui_SRCS_TaskBoxes
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
    TaskWatcher.cpp
)
SOURCE_GROUP("Task_Boxes" FILES ${RobotGui_SRCS_TaskBoxes})

SET(RobotGui_SRCS_TaskDlg
    TaskDlgSimulate.h
    TaskDlgSimulate.cpp
    TaskDlgEdge2Trac.h
    TaskDlgEdge2Trac.cpp
    TaskDlgTrajectoryCompound.h
    TaskDlgTrajectoryCompound.cpp
    TaskDlgTrajectoryDressUp.h
    TaskDlgTrajectoryDressUp.cpp
)
SOURCE_GROUP("Task_Dialogs" FILES ${RobotGui_SRCS_TaskDlg})

SET(RobotGui_SRCS
    ${RobotGui_UIC_HDRS}
    ${RobotResource_SRCS}
    ${RobotGui_SRCS_Module}
    ${RobotGui_SRCS_ViewProvider}
    ${RobotGui_SRCS_Commands}
    ${RobotGui_SRCS_TaskBoxes}
    ${RobotGui_SRCS_TaskDlg}
)

SET(RobotGuiIcon_SVG
    Resources/icons/RobotWorkbench.svg
)

add_library(RobotGui SHARED ${RobotGui_SRCS} ${RobotGuiIcon_SVG})

if(FREECAD_USE_PCH)
    target_precompile_headers(RobotGui PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    RobotGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(RobotGui ${RobotGui_LIBS})

if (MSVC)
    target_compile_options(RobotGui PRIVATE /wd4273)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(RobotGui)
endif()

unset(_flag_found CACHE)
check_cxx_compiler_flag("-Wno-deprecated-copy" _flag_found)
if (_flag_found)
    target_compile_options(RobotGui PRIVATE -Wno-deprecated-copy)
endif ()


SET_BIN_DIR(RobotGui RobotGui /Mod/Robot)
SET_PYTHON_PREFIX_SUFFIX(RobotGui)

fc_copy_sources(RobotGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Robot" ${RobotGuiIcon_SVG})

INSTALL(TARGETS RobotGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${RobotGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Robot/Resources/icons")
