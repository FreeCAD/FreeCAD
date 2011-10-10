if(MSVC)
    add_definitions(-DFCGuiRobot -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH -DEIGEN2_SUPPORT)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H -DEIGEN2_SUPPORT)
endif(MSVC)



include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(RobotGui_LIBS
    Robot
    FreeCADGui
)

qt4_add_resources(RobotResource_SRCS Resources/Robot.qrc)

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

qt4_wrap_ui(RobotGui_UIC_HDRS ${RobotGui_UIC_SRCS})

set(RobotGui_MOC_HDRS
    TaskTrajectory.h
    TaskRobot6Axis.h
    TrajectorySimulate.h
    TaskDlgSimulate.h
    TaskDlgEdge2Trac.h
    TaskDlgTrajectoryDressUp.h
    TaskDlgTrajectoryCompound.h
    TaskRobotMessages.h
    TaskRobotControl.h
    TaskEdge2TracParameter.h
    TaskTrajectoryDressUpParameter.h
    TaskWatcher.h
)
fc_wrap_cpp(RobotGui_MOC_SRCS ${RobotGui_MOC_HDRS})
SOURCE_GROUP("Qt" FILES ${PartDesignGui_MOC_SRCS})


SET(RobotGui_SRCS_Module
    AppRobotGui.cpp
    AppRobotGuiPy.cpp
    Resources/Robot.qrc
    PreCompiled.cpp
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



add_library(RobotGui SHARED ${RobotGui_SRCS})
target_link_libraries(RobotGui ${RobotGui_LIBS})
fc_copy_script("Mod/Robot" "RobotGui" InitGui.py)
fc_copy_script("Mod/Robot" "RobotGui" MovieTool.py)

if(MSVC)
    set_target_properties(RobotGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(RobotGui PROPERTIES DEBUG_OUTPUT_NAME "RobotGui_d")
    set_target_properties(RobotGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Robot)
    set_target_properties(RobotGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(RobotGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(RobotGui PROPERTIES DEBUG_OUTPUT_NAME "RobotGui_d")
    set_target_properties(RobotGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Robot)
    set_target_properties(RobotGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(RobotGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Robot)
    set_target_properties(RobotGui PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS RobotGui DESTINATION lib)
