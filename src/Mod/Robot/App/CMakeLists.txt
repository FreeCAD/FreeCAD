if(MSVC)
    add_definitions(-DFCAppRobot -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH -DEIGEN2_SUPPORT)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H -DEIGEN2_SUPPORT)
endif(MSVC)


include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

if(MSVC)
    set(Robot_LIBS
        Part
        ${QT_QTCORE_LIBRARY}
        ${QT_QTCORE_LIBRARY_DEBUG}
        FreeCADApp
    )
else(MSVC)
    set(Robot_LIBS
        Part
        ${QT_QTCORE_LIBRARY}
        FreeCADApp
    )
endif(MSVC)

FILE( GLOB KDL_SRCS kdl_cp/[^.]*.cpp )
FILE( GLOB KDL_HPPS kdl_cp/[^.]*.hpp kdl_cp/[^.]*.inl)

FILE( GLOB UTIL_SRCS kdl_cp/utilities/[^.]*.cpp kdl_cp/utilities/[^.]*.cxx)
FILE( GLOB UTIL_HPPS kdl_cp/utilities/[^.]*.h kdl_cp/utilities/[^.]*.hpp)

generate_from_xml(Robot6AxisPy)
generate_from_xml(TrajectoryPy)
generate_from_xml(WaypointPy)
generate_from_xml(RobotObjectPy)


SET(Python_SRCS
    Robot6AxisPy.xml
    Robot6AxisPyImp.cpp
    TrajectoryPy.xml
    TrajectoryPyImp.cpp 
    RobotObjectPy.xml
    RobotObjectPyImp.cpp
    WaypointPy.xml
    WaypointPyImp.cpp
)

SET(Mod_SRCS
    AppRobot.cpp
    AppRobotPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)

SET(Robot_SRCS
    RobotObject.cpp
    RobotObject.h
    TrajectoryObject.cpp
    TrajectoryObject.h
    TrajectoryDressUpObject.cpp
    TrajectoryDressUpObject.h
    TrajectoryCompound.cpp
    TrajectoryCompound.h
    Edge2TracObject.cpp
    Edge2TracObject.h
    PropertyTrajectory.cpp
    PropertyTrajectory.h
    RobotAlgos.cpp
    RobotAlgos.h
    Robot6Axis.cpp
    Robot6Axis.h
    Trajectory.cpp
    Trajectory.h
    Simulation.cpp
    Simulation.h
    Waypoint.cpp
    Waypoint.h
    ${KDL_SRCS}
    ${KDL_HPPS}
    ${UTIL_SRCS}
    ${UTIL_HPPS}
    ${Mod_SRCS}
    ${Python_SRCS}
)


SOURCE_GROUP("KDL" FILES ${KDL_SRCS} ${KDL_HPPS} ${UTIL_SRCS} ${UTIL_HPPS} )

SOURCE_GROUP("Python" FILES ${Python_SRCS})
SOURCE_GROUP("Module" FILES ${Mod_SRCS})

SET(Robot_Resources
    Lib/Kuka/kr500_1.wrl
    Lib/Kuka/kr500_1.csv
    Lib/Kuka/kr_500_2.pdf
    Lib/Kuka/kr210.WRL
    Lib/Kuka/kr_210_2.csv
    Lib/Kuka/kr_210_2.pdf
    Lib/Kuka/kr16.wrl
    Lib/Kuka/kr_16.csv
    Lib/Kuka/kr_16.pdf
    Lib/Kuka/kr125_3.wrl
    Lib/Kuka/kr_125.csv
    Lib/Kuka/kr125_2.pdf
    KukaExporter.py
    RobotExample.py
    RobotExampleTrajectoryOutOfShapes.py
)

add_library(Robot SHARED ${Robot_SRCS})
target_link_libraries(Robot ${Robot_LIBS})
fc_copy_script("Mod/Robot" "Robot" Init.py)
fc_copy_script("Mod/Robot" "Robot" ${Robot_Resources})

if(MSVC)
    set_target_properties(Robot PROPERTIES SUFFIX ".pyd")
    set_target_properties(Robot PROPERTIES DEBUG_OUTPUT_NAME "Robot_d")
    set_target_properties(Robot PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Robot)
    set_target_properties(Robot PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Robot PROPERTIES SUFFIX ".pyd")
    set_target_properties(Robot PROPERTIES DEBUG_OUTPUT_NAME "Robot_d")
    set_target_properties(Robot PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Robot)
    set_target_properties(Robot PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Robot PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Robot)
    set_target_properties(Robot PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS Robot DESTINATION lib)
