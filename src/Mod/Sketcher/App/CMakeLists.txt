if(MSVC)
    add_definitions(-DFCAppSketcher -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${QT_QTCORE_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(Sketcher_LIBS
    Part
    FreeCADApp
)

generate_from_xml(SketchObjectSFPy)
generate_from_xml(SketchObjectPy)
generate_from_xml(ConstraintPy)
generate_from_xml(SketchPy)


SET(Properties_SRCS
    PropertyConstraintList.cpp
    PropertyConstraintList.h
)
SOURCE_GROUP("Properties" FILES ${Properties_SRCS})


SET(Features_SRCS
    SketchObjectSF.cpp
    SketchObjectSF.h
    SketchObject.cpp
    SketchObject.h
)
SOURCE_GROUP("Features" FILES ${Features_SRCS})

SET(Datatypes_SRCS
    Constraint.cpp
    Constraint.h
    Sketch.cpp
    Sketch.h
)
SOURCE_GROUP("Datatypes" FILES ${Datatypes_SRCS})

SET(Python_SRCS
    SketchObjectSFPy.xml
    SketchObjectSFPyImp.cpp
    SketchObjectPy.xml
    SketchObjectPyImp.cpp
    ConstraintPyImp.cpp
    ConstraintPy.xml
    SketchPy.xml
    SketchPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})


SET(PlaneGCS_SRCS
    planegcs/qp_eq.h
    planegcs/GCS.cpp
    planegcs/GCS.h
    planegcs/Util.h
    planegcs/Geo.cpp
    planegcs/Geo.h
    planegcs/Constraints.cpp
    planegcs/Constraints.h
    planegcs/SubSystem.cpp
    planegcs/SubSystem.h
    planegcs/qp_eq.cpp
    planegcs/qp_eq.h
)
SOURCE_GROUP("PlaneGCS" FILES ${PlaneGCS_SRCS})

SET(SketchModule_SRCS
    AppSketcher.cpp
    AppSketcherPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${SketchModule_SRCS})

SET(Sketcher_SRCS
    ${Features_SRCS}
    ${PlaneGCS_SRCS}
    ${SketchModule_SRCS}
    ${Python_SRCS}
    ${Properties_SRCS}
    ${Datatypes_SRCS}
)

SET(Sketcher_Scripts
    Init.py
    SketcherExample.py
	TestSketcherApp.py
	Profiles.py
	ProfileLib/Hexagon.py
	ProfileLib/RegularPolygon.py
	ProfileLib/__init__.py
)

add_library(Sketcher SHARED ${Sketcher_SRCS})
target_link_libraries(Sketcher ${Sketcher_LIBS})


fc_target_copy_resource(Sketcher 
    ${CMAKE_SOURCE_DIR}/src/Mod/Sketcher
    ${CMAKE_BINARY_DIR}/Mod/Sketcher
    ${Sketcher_Scripts})

SET_BIN_DIR(Sketcher Sketcher /Mod/Sketcher)
SET_PYTHON_PREFIX_SUFFIX(Sketcher)

INSTALL(TARGETS Sketcher DESTINATION ${CMAKE_INSTALL_LIBDIR})
