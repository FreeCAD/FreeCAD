if(MSVC)
    add_definitions(-DFCAppRobot -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)


include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)
link_directories(${OCC_LIBRARY_DIR})

set(Robot_LIBS
    Part
    ${QT_QTCORE_LIBRARY}
    FreeCADApp
)

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
    ${Mod_SRCS}
    ${Python_SRCS}
)

# use external kdl
if (FREECAD_USE_EXTERNAL_KDL)
  find_library(KDL_LIBRARY orocos-kdl)
  find_path(KDL_INCLUDES kdl/kdl.hpp)
  if(KDL_LIBRARY)
     message(STATUS "Found orocos-kdl: ${KDL_LIBRARY}")
  endif()
  if(KDL_INCLUDES)
     message(STATUS "Found orocus_kdl headers: ${KDL_INCLUDES}")
  endif()
  if(KDL_LIBRARY AND KDL_INCLUDES)
      list(APPEND Robot_LIBS ${KDL_LIBRARY})
      include_directories(${KDL_INCLUDES})
  else()
      message(FATAL_ERROR "Using external orocos-kdl was specified but was not found.")
  endif()

else(FREECAD_USE_EXTERNAL_KDL)
  # here we use the internal supplied kdl
  FILE( GLOB KDL_SRCS kdl_cp/[^.]*.cpp )
  FILE( GLOB KDL_HPPS kdl_cp/[^.]*.hpp kdl_cp/[^.]*.inl)

  FILE( GLOB UTIL_SRCS kdl_cp/utilities/[^.]*.cpp kdl_cp/utilities/[^.]*.cxx)
  FILE( GLOB UTIL_HPPS kdl_cp/utilities/[^.]*.h kdl_cp/utilities/[^.]*.hpp)

  SET(Robot_SRCS
      ${Robot_SRCS}
      ${KDL_SRCS}
      ${KDL_HPPS}
      ${UTIL_SRCS}
      ${UTIL_HPPS}
  )

  SOURCE_GROUP("KDL" FILES ${KDL_SRCS} ${KDL_HPPS} ${UTIL_SRCS} ${UTIL_HPPS} )

endif(FREECAD_USE_EXTERNAL_KDL)

SOURCE_GROUP("Python" FILES ${Python_SRCS})
SOURCE_GROUP("Module" FILES ${Mod_SRCS})

SET(Robot_Scripts
    Init.py
    KukaExporter.py
    RobotExample.py
    RobotExampleTrajectoryOutOfShapes.py
)

if (EXISTS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/Lib/Kuka)
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
)
endif ()

add_library(Robot SHARED ${Robot_SRCS})
target_link_libraries(Robot ${Robot_LIBS})

fc_target_copy_resource(Robot 
    ${CMAKE_SOURCE_DIR}/src/Mod/Robot
    ${CMAKE_BINARY_DIR}/Mod/Robot
    ${Robot_Scripts})

fc_target_copy_resource(Robot 
    ${CMAKE_SOURCE_DIR}/src/Mod/Robot
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Robot
    ${Robot_Resources})

SET_BIN_DIR(Robot Robot /Mod/Robot)
SET_PYTHON_PREFIX_SUFFIX(Robot)

INSTALL(TARGETS Robot DESTINATION ${CMAKE_INSTALL_LIBDIR})
