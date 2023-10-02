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

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Robot_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Robot PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

# FIXME: The bundled KDL has some extensions which makes it incompatible
# to an installed KDL. To fix the issue two things must be done:
# * revert the changes (and look for an alternative)
# * it must be avoided to include headers of the bundled version if the
#   installed version is used
#
# use external kdl
#if (FREECAD_USE_EXTERNAL_KDL)
#  find_library(KDL_LIBRARY orocos-kdl)
#  find_path(KDL_INCLUDES kdl/kdl.hpp)
#  if(KDL_LIBRARY)
#     message(STATUS "Found orocos-kdl: ${KDL_LIBRARY}")
#  endif()
#  if(KDL_INCLUDES)
#     message(STATUS "Found orocus_kdl headers: ${KDL_INCLUDES}")
#  endif()
#  if(KDL_LIBRARY AND KDL_INCLUDES)
#      list(APPEND Robot_LIBS ${KDL_LIBRARY})
#      include_directories(${KDL_INCLUDES})
#  else()
#      message(FATAL_ERROR "Using external orocos-kdl was specified but was not found.")
#  endif()
#
#else(FREECAD_USE_EXTERNAL_KDL)
  # here we use the internal supplied kdl
  add_definitions(-DKDL_USE_NEW_TREE_INTERFACE=1)
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

#endif(FREECAD_USE_EXTERNAL_KDL)

SOURCE_GROUP("Python" FILES ${Python_SRCS})
SOURCE_GROUP("Module" FILES ${Mod_SRCS})

add_library(Robot SHARED ${Robot_SRCS})
target_link_libraries(Robot ${Robot_LIBS})

unset(_flag_found CACHE)
check_cxx_compiler_flag("-Wno-deprecated-copy" _flag_found)
if (_flag_found)
  target_compile_options(Robot PRIVATE -Wno-deprecated-copy)
endif()

if(MINGW)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-all-symbols")
endif()

SET_BIN_DIR(Robot Robot /Mod/Robot)
SET_PYTHON_PREFIX_SUFFIX(Robot)

INSTALL(TARGETS Robot DESTINATION ${CMAKE_INSTALL_LIBDIR})
