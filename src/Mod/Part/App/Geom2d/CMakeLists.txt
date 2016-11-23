generate_from_xml(ArcOfCircle2dPy)
generate_from_xml(ArcOfConic2dPy)
generate_from_xml(ArcOfEllipse2dPy)
generate_from_xml(ArcOfHyperbola2dPy)
generate_from_xml(ArcOfParabola2dPy)
generate_from_xml(BezierCurve2dPy)
generate_from_xml(BSplineCurve2dPy)
generate_from_xml(Circle2dPy)
generate_from_xml(Conic2dPy)
generate_from_xml(Ellipse2dPy)
generate_from_xml(Geometry2dPy)
generate_from_xml(Hyperbola2dPy)
generate_from_xml(Curve2dPy)
generate_from_xml(Line2dSegmentPy)
generate_from_xml(OffsetCurve2dPy)
generate_from_xml(Parabola2dPy)

# Geom2d wrappers
SET(Geom2dPy_SRCS
    ArcOfCircle2dPy.xml
    ArcOfCircle2dPyImp.cpp
    ArcOfConic2dPy.xml
    ArcOfConic2dPyImp.cpp
    ArcOfEllipse2dPy.xml
    ArcOfEllipse2dPyImp.cpp
    ArcOfHyperbola2dPy.xml
    ArcOfHyperbola2dPyImp.cpp
    ArcOfParabola2dPy.xml
    ArcOfParabola2dPyImp.cpp
    BezierCurve2dPy.xml
    BezierCurve2dPyImp.cpp
    BSplineCurve2dPy.xml
    BSplineCurve2dPyImp.cpp
    Circle2dPy.xml
    Circle2dPyImp.cpp
    Conic2dPy.xml
    Conic2dPyImp.cpp
    Ellipse2dPy.xml
    Ellipse2dPyImp.cpp
    Geometry2dPy.xml
    Geometry2dPyImp.cpp
    Curve2dPy.xml
    Curve2dPyImp.cpp
    Hyperbola2dPy.xml
    Hyperbola2dPyImp.cpp
    Line2dSegmentPy.xml
    Line2dSegmentPyImp.cpp
    OffsetCurve2dPy.xml
    OffsetCurve2dPyImp.cpp
    Parabola2dPy.xml
    Parabola2dPyImp.cpp
)

SOURCE_GROUP("Geom2d" FILES ${Geom2dPy_SRCS})

# https://cmake.org/Wiki/CMake/Tutorials/Object_Library
if(CMAKE_VERSION VERSION_LESS 2.8.8)
    add_library(PartGeom2dPy ${Geom2dPy_SRCS})
else()
    add_library(PartGeom2dPy OBJECT ${Geom2dPy_SRCS})
endif()
