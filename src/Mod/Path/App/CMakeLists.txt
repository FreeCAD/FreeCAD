if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_definitions(-fext-numeric-literals) #fix for gcc and qt5
    endif()
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
    ${XercesC_INCLUDE_DIRS}
)
link_directories(${OCC_LIBRARY_DIR})

set(Path_LIBS
#   Robot
    Part
    area-native
    FreeCADApp
)

generate_from_xml(CommandPy)
generate_from_xml(PathPy)
generate_from_xml(ToolPy)
generate_from_xml(TooltablePy)
generate_from_xml(FeaturePathCompoundPy)
generate_from_xml(AreaPy)
generate_from_xml(FeatureAreaPy)

SET(Python_SRCS
    CommandPy.xml
    CommandPyImp.cpp
    PathPy.xml
    PathPyImp.cpp
    ToolPy.xml
    TooltablePy.xml
    TooltablePyImp.cpp
    FeaturePathCompoundPy.xml
    FeaturePathCompoundPyImp.cpp
    AreaPy.xml
    AreaPyImp.cpp
    FeatureAreaPy.xml
    FeatureAreaPyImp.cpp
)

SET(Mod_SRCS
    AppPath.cpp
    AppPathPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)

SET(Path_SRCS
    Command.cpp
    Command.h
    Path.cpp
    Path.h
    Tooltable.cpp
    Tooltable.h
    PropertyPath.cpp
    PropertyPath.h
    FeaturePath.cpp
    FeaturePath.h
    PropertyTool.cpp
    PropertyTool.h
    PropertyTooltable.cpp
    PropertyTooltable.h
    FeaturePathCompound.cpp
    FeaturePathCompound.h
    FeaturePathShape.cpp
    FeaturePathShape.h
    Area.cpp
    Area.h
    AreaParams.h
    ParamsHelper.h
    FeatureArea.cpp
    FeatureArea.h
    ${Mod_SRCS}
    ${Python_SRCS}
)

SOURCE_GROUP("Python" FILES ${Python_SRCS})
SOURCE_GROUP("Module" FILES ${Mod_SRCS})

#if (WIN32) uncomment to use KDL
#    FILE( GLOB KDL_SRCS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.cpp )
#    FILE( GLOB KDL_HPPS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.hpp
#                        ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/[^.]*.inl)
#
#    FILE( GLOB UTIL_SRCS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.cpp
#                         ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.cxx)
#    FILE( GLOB UTIL_HPPS ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.h
#                         ${CMAKE_SOURCE_DIR}/src/Mod/Robot/App/kdl_cp/utilities/[^.]*.hpp)
#
#    SET(Path_SRCS
#        ${Path_SRCS}
#        ${KDL_SRCS}
#        ${KDL_HPPS}
#        ${UTIL_SRCS}
#        ${UTIL_HPPS}
#    )
#
#    SOURCE_GROUP("KDL" FILES ${KDL_SRCS} ${KDL_HPPS} ${UTIL_SRCS} ${UTIL_HPPS} )
#endif(WIN32)

add_library(Path SHARED ${Path_SRCS})
target_link_libraries(Path ${Path_LIBS})

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" Path_CPP_SRCS ${Path_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Path PreCompiled.h PreCompiled.cpp Path_CPP_SRCS)
endif(FREECAD_USE_PCH)


SET_BIN_DIR(Path Path /Mod/Path)
SET_PYTHON_PREFIX_SUFFIX(Path)

INSTALL(TARGETS Path DESTINATION ${CMAKE_INSTALL_LIBDIR})
