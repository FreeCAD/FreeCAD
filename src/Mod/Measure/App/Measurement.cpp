/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <gp_Pln.hxx>
#include <gp_Circ.hxx>
#include <gp_Torus.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Sphere.hxx>
#include <gp_Lin.hxx>
#include <GProp_GProps.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#endif


#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include "Measurement.h"
#include "MeasurementPy.h"


using namespace Measure;
using namespace Base;
using namespace Part;

TYPESYSTEM_SOURCE(Measure::Measurement, Base::BaseClass)

Measurement::Measurement()
{
    measureType = MeasureType::Invalid;
    References3D.setScope(App::LinkScope::Global);
}

Measurement::~Measurement() = default;

void Measurement::clear()
{
    std::vector<App::DocumentObject*> Objects;
    std::vector<std::string> SubElements;
    References3D.setValues(Objects, SubElements);
    measureType = MeasureType::Invalid;
}

bool Measurement::has3DReferences()
{
    return (References3D.getSize() > 0);
}

// add a 3D reference (obj+sub) to end of list
int Measurement::addReference3D(App::DocumentObject* obj, const std::string& subName)
{
    return addReference3D(obj, subName.c_str());
}

/// add a 3D reference (obj+sub) to end of list
int Measurement::addReference3D(App::DocumentObject* obj, const char* subName)
{
    std::vector<App::DocumentObject*> objects = References3D.getValues();
    std::vector<std::string> subElements = References3D.getSubValues();

    objects.push_back(obj);
    subElements.emplace_back(subName);

    References3D.setValues(objects, subElements);

    measureType = findType();
    return References3D.getSize();
}

MeasureType Measurement::findType()
{
    const std::vector<App::DocumentObject*>& objects = References3D.getValues();
    const std::vector<std::string>& subElements = References3D.getSubValues();

    std::vector<App::DocumentObject*>::const_iterator obj = objects.begin();
    std::vector<std::string>::const_iterator subEl = subElements.begin();

    MeasureType mode;

    int verts = 0;
    int edges = 0;
    int lines = 0;
    int circles = 0;
    int faces = 0;
    int planes = 0;
    int cylinders = 0;
    int cones = 0;
    int torus = 0;
    int spheres = 0;
    int vols = 0;
    int other = 0;

    for (; obj != objects.end(); ++obj, ++subEl) {
        TopoDS_Shape refSubShape;
        try {
            refSubShape = Part::Feature::getShape(*obj, (*subEl).c_str(), true);
            if (refSubShape.IsNull()) {
                return MeasureType::Invalid;
            }
        }
        catch (Standard_Failure& e) {
            std::stringstream errorMsg;

            errorMsg << "Measurement - getType - " << e.GetMessageString() << std::endl;
            throw Base::CADKernelError(e.GetMessageString());
        }

        switch (refSubShape.ShapeType()) {
            case TopAbs_VERTEX: {
                verts++;
            } break;
            case TopAbs_EDGE: {
                edges++;
                TopoDS_Edge edge = TopoDS::Edge(refSubShape);
                BRepAdaptor_Curve sf(edge);

                if (sf.GetType() == GeomAbs_Line) {
                    lines++;
                }
                else if (sf.GetType() == GeomAbs_Circle) {
                    circles++;
                }
            } break;
            case TopAbs_FACE: {
                faces++;
                TopoDS_Face face = TopoDS::Face(refSubShape);
                BRepAdaptor_Surface sf(face);

                if (sf.GetType() == GeomAbs_Plane) {
                    planes++;
                }
                else if (sf.GetType() == GeomAbs_Cylinder) {
                    cylinders++;
                }
                else if (sf.GetType() == GeomAbs_Sphere) {
                    spheres++;
                }
                else if (sf.GetType() == GeomAbs_Cone) {
                    cones++;
                }
                else if (sf.GetType() == GeomAbs_Torus) {
                    torus++;
                }
            } break;
            case TopAbs_SOLID: {
                vols++;
            } break;
            default:
                other++;
                break;
        }
    }

    if (other > 0) {
        mode = MeasureType::Invalid;
    }
    else if (vols > 0) {
        if (verts > 0 || edges > 0 || faces > 0) {
            mode = MeasureType::Invalid;
        }
        else {
            mode = MeasureType::Volumes;
        }
    }
    else if (faces > 0) {
        if (verts > 0 || edges > 0) {
            if (faces == 1 && verts == 1) {
                mode = MeasureType::PointToSurface;
            }
            else {
                mode = MeasureType::Invalid;
            }
        }
        else {
            if (planes == 1 && faces == 1) {
                mode = MeasureType::Plane;
            }
            else if (planes == 2 && faces == 2) {
                if (planesAreParallel()) {
                    mode = MeasureType::TwoPlanes;
                }
                else {
                    mode = MeasureType::Surfaces;
                }
            }
            else if (cylinders == 1 && faces == 1) {
                mode = MeasureType::Cylinder;
            }
            else if (cones == 1 && faces == 1) {
                mode = MeasureType::Cone;
            }
            else if (spheres == 1 && faces == 1) {
                mode = MeasureType::Sphere;
            }
            else if (torus == 1 && faces == 1) {
                mode = MeasureType::Torus;
            }
            else {
                mode = MeasureType::Surfaces;
            }
        }
    }
    else if (edges > 0) {
        if (verts > 0) {
            if (verts > 1 && edges > 0) {
                mode = MeasureType::Invalid;
            }
            else {
                mode = MeasureType::PointToEdge;
            }
        }
        else if (lines == 1 && edges == 1) {
            mode = MeasureType::Line;
        }
        else if (lines == 2 && edges == 2) {
            if (linesAreParallel()) {
                mode = MeasureType::TwoParallelLines;
            }
            else {
                mode = MeasureType::TwoLines;
            }
        }
        else if (circles == 1 && edges == 1) {
            mode = MeasureType::Circle;
        }
        else {
            mode = MeasureType::Edges;
        }
    }
    else if (verts > 0) {
        if (verts == 2) {
            mode = MeasureType::PointToPoint;
        }
        else {
            mode = MeasureType::Points;
        }
    }
    else {
        mode = MeasureType::Invalid;
    }

    return mode;
}

MeasureType Measurement::getType()
{
    return measureType;
}

TopoDS_Shape Measurement::getShape(App::DocumentObject* rootObj, const char* subName) const
{
    std::vector<std::string> names = Base::Tools::splitSubName(subName);

    if (names.empty()) {
        TopoDS_Shape shape = Part::Feature::getShape(rootObj);
        if (shape.IsNull()) {
            throw Part::NullShapeException("null shape in measurement");
        }
        return shape;
    }

    try {
        App::DocumentObject* obj = rootObj->getSubObject(subName);

        Part::TopoShape partShape = Part::Feature::getTopoShape(obj);

        partShape.setPlacement(App::GeoFeature::getGlobalPlacement(obj, rootObj, subName));

        TopoDS_Shape shape = partShape.getSubShape(names.back().c_str());
        if (shape.IsNull()) {
            throw Part::NullShapeException("null shape in measurement");
        }
        return shape;
    }
    catch (const Base::Exception&) {
        // re-throw original exception
        throw;
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
    catch (...) {
        throw Base::RuntimeError("Measurement: Unknown error retrieving shape");
    }
}

// TODO:: add lengthX, lengthY (and lengthZ??) support
//  Methods for distances (edge length, two points, edge and a point
double Measurement::length() const
{
    double result = 0.0;
    int numRefs = References3D.getSize();
    if (numRefs == 0) {
        Base::Console().Error("Measurement::length - No 3D references available\n");
    }
    else if (measureType == MeasureType::Invalid) {
        Base::Console().Error("Measurement::length - measureType is Invalid\n");
    }
    else {
        const std::vector<App::DocumentObject*>& objects = References3D.getValues();
        const std::vector<std::string>& subElements = References3D.getSubValues();

        if (measureType == MeasureType::Points || measureType == MeasureType::PointToPoint
            || measureType == MeasureType::PointToEdge
            || measureType == MeasureType::PointToSurface) {

            Base::Vector3d diff = this->delta();
            result = diff.Length();
        }
        else if (measureType == MeasureType::Edges || measureType == MeasureType::Line
                 || measureType == MeasureType::TwoLines || measureType == MeasureType::Circle) {

            // Iterate through edges and calculate each length
            std::vector<App::DocumentObject*>::const_iterator obj = objects.begin();
            std::vector<std::string>::const_iterator subEl = subElements.begin();

            for (; obj != objects.end(); ++obj, ++subEl) {

                //  Get the length of one edge
                TopoDS_Shape shape = getShape(*obj, (*subEl).c_str());
                const TopoDS_Edge& edge = TopoDS::Edge(shape);
                BRepAdaptor_Curve curve(edge);

                switch (curve.GetType()) {
                    case GeomAbs_Line: {
                        gp_Pnt P1 = curve.Value(curve.FirstParameter());
                        gp_Pnt P2 = curve.Value(curve.LastParameter());
                        gp_XYZ diff = P2.XYZ() - P1.XYZ();
                        result += diff.Modulus();
                        break;
                    }
                    case GeomAbs_Circle: {
                        double u = curve.FirstParameter();
                        double v = curve.LastParameter();
                        double radius = curve.Circle().Radius();
                        if (u > v) {  // if arc is reversed
                            std::swap(u, v);
                        }

                        double range = v - u;
                        result += radius * range;
                        break;
                    }
                    case GeomAbs_Ellipse:
                    case GeomAbs_BSplineCurve:
                    case GeomAbs_Hyperbola:
                    case GeomAbs_BezierCurve: {
                        result += GCPnts_AbscissaPoint::Length(curve);
                        break;
                    }
                    default: {
                        throw Base::RuntimeError(
                            "Measurement - length - Curve type not currently handled");
                    }
                }  // end switch
            }  // end for
        }
    }
    return result;
}

double Measurement::lineLineDistance() const
{
    // We don't use delta() because BRepExtrema_DistShapeShape return minimum length between line
    // segment. Here we get the nominal distance between the infinite lines.
    double distance = 0.0;

    if (measureType != MeasureType::TwoParallelLines || References3D.getSize() != 2) {
        return distance;
    }

    const std::vector<App::DocumentObject*>& objects = References3D.getValues();
    const std::vector<std::string>& subElements = References3D.getSubValues();

    // Get the first line
    TopoDS_Shape shape1 = getShape(objects[0], subElements[0].c_str());
    const TopoDS_Edge& edge1 = TopoDS::Edge(shape1);
    BRepAdaptor_Curve curve1(edge1);

    // Get the second line
    TopoDS_Shape shape2 = getShape(objects[1], subElements[1].c_str());
    const TopoDS_Edge& edge2 = TopoDS::Edge(shape2);
    BRepAdaptor_Curve curve2(edge2);

    if (curve1.GetType() == GeomAbs_Line && curve2.GetType() == GeomAbs_Line) {
        gp_Lin line1 = curve1.Line();
        gp_Lin line2 = curve2.Line();

        gp_Pnt p1 = line1.Location();
        gp_Pnt p2 = line2.Location();

        // Create a vector from a point on line1 to a point on line2
        gp_Vec lineVec(p1, p2);

        // The direction vector of one of the lines
        gp_Dir lineDir = line1.Direction();

        // Project lineVec onto lineDir
        gp_Vec parallelComponent = lineVec.Dot(lineDir) * lineDir;

        // Compute the perpendicular component
        gp_Vec perpendicularComponent = lineVec - parallelComponent;

        // Distance is the magnitude of the perpendicular component
        distance = perpendicularComponent.Magnitude();
    }
    else {
        Base::Console().Error("Measurement::length - TwoLines measureType requires two lines\n");
    }
    return distance;
}

double Measurement::planePlaneDistance() const
{
    if (measureType != MeasureType::TwoPlanes || References3D.getSize() != 2) {
        return 0.0;
    }

    const auto& objects = References3D.getValues();
    const auto& subElements = References3D.getSubValues();

    std::vector<gp_Pln> planes;

    // Get the first plane
    TopoDS_Shape shape1 = getShape(objects[0], subElements[0].c_str());
    const TopoDS_Face& face1 = TopoDS::Face(shape1);
    BRepAdaptor_Surface surface1(face1);
    const gp_Pln& plane1 = surface1.Plane();

    // Get the second plane
    TopoDS_Shape shape2 = getShape(objects[1], subElements[1].c_str());
    const TopoDS_Face& face2 = TopoDS::Face(shape2);
    BRepAdaptor_Surface surface2(face2);
    const gp_Pln& plane2 = surface2.Plane();

    // Distance between two parallel planes
    gp_Pnt pointOnPlane1 = plane1.Location();
    gp_Dir normalToPlane1 = plane1.Axis().Direction();

    gp_Pnt pointOnPlane2 = plane2.Location();

    // Create a vector from a point on plane1 to a point on plane2
    gp_Vec vectorBetweenPlanes(pointOnPlane1, pointOnPlane2);

    // Project this vector onto the plane normal
    double distance = Abs(vectorBetweenPlanes.Dot(normalToPlane1));

    return distance;
}

double Measurement::angle(const Base::Vector3d& /*param*/) const
{
    // TODO: do these references arrive as obj+sub pairs or as a struct of obj + [subs]?
    const std::vector<App::DocumentObject*>& objects = References3D.getValues();
    const std::vector<std::string>& subElements = References3D.getSubValues();
    int numRefs = objects.size();
    if (numRefs == 0) {
        throw Base::RuntimeError("No references available for angle measurement");
    }
    else if (measureType == MeasureType::Invalid) {
        throw Base::RuntimeError("MeasureType is Invalid for angle measurement");
    }
    else if (measureType == MeasureType::TwoLines) {
        // Only case that is supported is edge to edge
        // The angle between two skew lines is measured by the angle between one line (A)
        // and a line (B) with the direction of the second through a point on the first line.
        // Since we don't know if the directions of the lines point in the same general direction
        // we could get the angle we want or the supplementary angle.
        if (numRefs == 2) {
            TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
            TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());

            BRepAdaptor_Curve curve1(TopoDS::Edge(shape1));
            BRepAdaptor_Curve curve2(TopoDS::Edge(shape2));

            if (curve1.GetType() == GeomAbs_Line && curve2.GetType() == GeomAbs_Line) {

                gp_Pnt pnt1First = curve1.Value(curve1.FirstParameter());
                gp_Dir dir1 = curve1.Line().Direction();
                gp_Dir dir2 = curve2.Line().Direction();
                gp_Dir dir2r = curve2.Line().Direction().Reversed();

                gp_Lin l1 = gp_Lin(pnt1First, dir1);    // (A)
                gp_Lin l2 = gp_Lin(pnt1First, dir2);    // (B)
                gp_Lin l2r = gp_Lin(pnt1First, dir2r);  // (B')
                Standard_Real aRad = l1.Angle(l2);
                double aRadr = l1.Angle(l2r);
                return Base::toDegrees<double>(std::min(aRad, aRadr));
            }
            else {
                throw Base::RuntimeError("Measurement references must both be lines");
            }
        }
        else {
            throw Base::RuntimeError("Can not compute angle measurement - too many references");
        }
    }
    else if (measureType == MeasureType::Points) {
        // NOTE: we are calculating the 3d angle here, not the projected angle
        // ASSUMPTION: the references are in end-apex-end order
        if (numRefs == 3) {
            TopoDS_Shape shape0 = getShape(objects.at(0), subElements.at(0).c_str());
            TopoDS_Shape shape1 = getShape(objects.at(1), subElements.at(1).c_str());
            TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(2).c_str());
            if (shape0.ShapeType() != TopAbs_VERTEX || shape1.ShapeType() != TopAbs_VERTEX
                || shape2.ShapeType() != TopAbs_VERTEX) {
                throw Base::RuntimeError("Measurement references for 3 point angle are not Vertex");
            }
            gp_Pnt gEnd0 = BRep_Tool::Pnt(TopoDS::Vertex(shape0));
            gp_Pnt gApex = BRep_Tool::Pnt(TopoDS::Vertex(shape1));
            gp_Pnt gEnd1 = BRep_Tool::Pnt(TopoDS::Vertex(shape2));
            gp_Dir gDir0 = gp_Dir(gEnd0.XYZ() - gApex.XYZ());
            gp_Dir gDir1 = gp_Dir(gEnd1.XYZ() - gApex.XYZ());
            gp_Lin line0 = gp_Lin(gEnd0, gDir0);
            gp_Lin line1 = gp_Lin(gEnd1, gDir1);
            double radians = line0.Angle(line1);
            return Base::toDegrees<double>(radians);
        }
    }
    throw Base::RuntimeError("Unexpected error for angle measurement");
}

double Measurement::radius() const
{
    const std::vector<App::DocumentObject*>& objects = References3D.getValues();
    const std::vector<std::string>& subElements = References3D.getSubValues();

    int numRefs = References3D.getSize();
    if (numRefs == 0) {
        Base::Console().Error("Measurement::radius - No 3D references available\n");
    }
    else if (measureType == MeasureType::Circle) {
        TopoDS_Shape shape = getShape(objects.at(0), subElements.at(0).c_str());
        const TopoDS_Edge& edge = TopoDS::Edge(shape);

        BRepAdaptor_Curve curve(edge);
        if (curve.GetType() == GeomAbs_Circle) {
            return (double)curve.Circle().Radius();
        }
    }
    else if (measureType == MeasureType::Cylinder || measureType == MeasureType::Sphere
             || measureType == MeasureType::Torus) {
        TopoDS_Shape shape = getShape(objects.at(0), subElements.at(0).c_str());
        TopoDS_Face face = TopoDS::Face(shape);

        BRepAdaptor_Surface sf(face);
        if (sf.GetType() == GeomAbs_Cylinder) {
            return sf.Cylinder().Radius();
        }
        else if (sf.GetType() == GeomAbs_Sphere) {
            return sf.Sphere().Radius();
        }
        else if (sf.GetType() == GeomAbs_Torus) {
            return sf.Torus().MinorRadius();
        }
    }
    Base::Console().Error("Measurement::radius - Invalid References3D Provided\n");
    return 0.0;
}

Base::Vector3d Measurement::delta() const
{
    Base::Vector3d result;
    int numRefs = References3D.getSize();
    if (numRefs == 0) {
        Base::Console().Error("Measurement::delta - No 3D references available\n");
    }
    else if (measureType == MeasureType::Invalid) {
        Base::Console().Error("Measurement::delta - measureType is Invalid\n");
    }
    else {
        const std::vector<App::DocumentObject*>& objects = References3D.getValues();
        const std::vector<std::string>& subElements = References3D.getSubValues();

        if (measureType == MeasureType::PointToPoint) {
            if (numRefs == 2) {
                // Keep separate case for two points to reduce need for complex algorithm
                TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());

                const TopoDS_Vertex& vert1 = TopoDS::Vertex(shape1);
                const TopoDS_Vertex& vert2 = TopoDS::Vertex(shape2);

                gp_Pnt P1 = BRep_Tool::Pnt(vert1);
                gp_Pnt P2 = BRep_Tool::Pnt(vert2);
                gp_XYZ diff = P2.XYZ() - P1.XYZ();
                return Base::Vector3d(diff.X(), diff.Y(), diff.Z());
            }
        }
        else if (measureType == MeasureType::PointToEdge
                 || measureType == MeasureType::PointToSurface) {
            // BrepExtema can calculate minimum distance between any set of topology sets.
            if (numRefs == 2) {
                TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());

                BRepExtrema_DistShapeShape extrema(shape1, shape2);

                if (extrema.IsDone()) {
                    // Found the nearest point between point and curve
                    // NOTE we will assume there is only 1 solution (cyclic topology will create
                    // multiple solutions.
                    gp_Pnt P1 = extrema.PointOnShape1(1);
                    gp_Pnt P2 = extrema.PointOnShape2(1);
                    gp_XYZ diff = P2.XYZ() - P1.XYZ();
                    result = Base::Vector3d(diff.X(), diff.Y(), diff.Z());
                }
            }
        }
        else if (measureType == MeasureType::Edges) {
            // Only case that is supported is straight line edge
            if (numRefs == 1) {
                TopoDS_Shape shape = getShape(objects.at(0), subElements.at(0).c_str());
                const TopoDS_Edge& edge = TopoDS::Edge(shape);
                BRepAdaptor_Curve curve(edge);

                if (curve.GetType() == GeomAbs_Line) {
                    gp_Pnt P1 = curve.Value(curve.FirstParameter());
                    gp_Pnt P2 = curve.Value(curve.LastParameter());
                    gp_XYZ diff = P2.XYZ() - P1.XYZ();
                    result = Base::Vector3d(diff.X(), diff.Y(), diff.Z());
                }
            }
            else if (numRefs == 2) {
                TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());

                BRepAdaptor_Curve curve1(TopoDS::Edge(shape1));
                BRepAdaptor_Curve curve2(TopoDS::Edge(shape2));

                // Only permit line to line distance
                if (curve1.GetType() == GeomAbs_Line && curve2.GetType() == GeomAbs_Line) {
                    BRepExtrema_DistShapeShape extrema(shape1, shape2);

                    if (extrema.IsDone()) {
                        // Found the nearest point between point and curve
                        // NOTE we will assume there is only 1 solution (cyclic topology will create
                        // multiple solutions.
                        gp_Pnt P1 = extrema.PointOnShape1(1);
                        gp_Pnt P2 = extrema.PointOnShape2(1);
                        gp_XYZ diff = P2.XYZ() - P1.XYZ();
                        result = Base::Vector3d(diff.X(), diff.Y(), diff.Z());
                    }
                }
            }
        }
        else {
            Base::Console().Error("Measurement::delta - measureType is not recognized\n");
        }
    }
    return result;
}

double Measurement::volume() const
{
    double result = 0.0;
    if (References3D.getSize() == 0) {
        Base::Console().Error("Measurement::volume - No 3D references available\n");
    }
    else if (measureType != MeasureType::Volumes) {
        Base::Console().Error("Measurement::volume - measureType is not Volumes\n");
    }
    else {
        const std::vector<App::DocumentObject*>& objects = References3D.getValues();
        const std::vector<std::string>& subElements = References3D.getSubValues();

        for (size_t i = 0; i < objects.size(); ++i) {
            GProp_GProps props = GProp_GProps();
            BRepGProp::VolumeProperties(getShape(objects[i], subElements[i].c_str()), props);
            result += props.Mass();
        }
    }
    return result;
}

double Measurement::area() const
{
    double result = 0.0;
    if (References3D.getSize() == 0) {
        Base::Console().Error("Measurement::area - No 3D references available\n");
    }
    else if (measureType == MeasureType::Volumes || measureType == MeasureType::Surfaces
             || measureType == MeasureType::Cylinder || measureType == MeasureType::Cone
             || measureType == MeasureType::Sphere || measureType == MeasureType::Torus
             || measureType == MeasureType::Plane) {

        const std::vector<App::DocumentObject*>& objects = References3D.getValues();
        const std::vector<std::string>& subElements = References3D.getSubValues();

        for (size_t i = 0; i < objects.size(); ++i) {
            GProp_GProps props;
            BRepGProp::SurfaceProperties(getShape(objects[i], subElements[i].c_str()), props);
            result += props.Mass();  // Area is obtained using Mass method for surface properties
        }
    }
    else {
        Base::Console().Error("Measurement::area - measureType is not valid\n");
    }
    return result;
}

Base::Vector3d Measurement::massCenter() const
{
    Base::Vector3d result;
    int numRefs = References3D.getSize();
    if (numRefs == 0) {
        Base::Console().Error("Measurement::massCenter - No 3D references available\n");
    }
    else if (measureType == MeasureType::Invalid) {
        Base::Console().Error("Measurement::massCenter - measureType is Invalid\n");
    }
    else {
        const std::vector<App::DocumentObject*>& objects = References3D.getValues();
        const std::vector<std::string>& subElements = References3D.getSubValues();
        GProp_GProps gprops = GProp_GProps();

        if (measureType == MeasureType::Volumes) {
            // Iterate through edges and calculate each length
            std::vector<App::DocumentObject*>::const_iterator obj = objects.begin();
            std::vector<std::string>::const_iterator subEl = subElements.begin();

            for (; obj != objects.end(); ++obj, ++subEl) {

                // Compute inertia properties

                GProp_GProps props = GProp_GProps();
                BRepGProp::VolumeProperties(getShape((*obj), ""), props);
                gprops.Add(props);
                // Get inertia properties
            }

            gp_Pnt cog = gprops.CentreOfMass();

            return Base::Vector3d(cog.X(), cog.Y(), cog.Z());
        }
        else {
            Base::Console().Error("Measurement::massCenter - measureType is not recognized\n");
        }
    }
    return result;
}

bool Measurement::planesAreParallel() const
{
    const std::vector<App::DocumentObject*>& objects = References3D.getValues();
    const std::vector<std::string>& subElements = References3D.getSubValues();

    std::vector<gp_Dir> planeNormals;

    for (size_t i = 0; i < objects.size(); ++i) {
        TopoDS_Shape refSubShape;
        try {
            refSubShape = Part::Feature::getShape(objects[i], subElements[i].c_str(), true);
            if (refSubShape.IsNull()) {
                return false;
            }
        }
        catch (Standard_Failure& e) {
            std::stringstream errorMsg;
            errorMsg << "Measurement - planesAreParallel - " << e.GetMessageString() << std::endl;
            throw Base::CADKernelError(e.GetMessageString());
        }

        if (refSubShape.ShapeType() == TopAbs_FACE) {
            TopoDS_Face face = TopoDS::Face(refSubShape);
            BRepAdaptor_Surface sf(face);

            if (sf.GetType() == GeomAbs_Plane) {
                gp_Pln plane = sf.Plane();
                gp_Dir normal = plane.Axis().Direction();
                planeNormals.push_back(normal);
            }
        }
    }

    if (planeNormals.size() != 2) {
        return false;  // Ensure exactly two planes are considered
    }

    // Check if normals are parallel (either identical or opposite)
    const gp_Dir& normal1 = planeNormals[0];
    const gp_Dir& normal2 = planeNormals[1];

    return normal1.IsParallel(normal2, Precision::Angular());
}

bool Measurement::linesAreParallel() const
{
    const std::vector<App::DocumentObject*>& objects = References3D.getValues();
    const std::vector<std::string>& subElements = References3D.getSubValues();

    if (References3D.getSize() != 2) {
        return false;
    }

    // Get the first line
    TopoDS_Shape shape1 = getShape(objects[0], subElements[0].c_str());
    const TopoDS_Edge& edge1 = TopoDS::Edge(shape1);
    BRepAdaptor_Curve curve1(edge1);

    // Get the second line
    TopoDS_Shape shape2 = getShape(objects[1], subElements[1].c_str());
    const TopoDS_Edge& edge2 = TopoDS::Edge(shape2);
    BRepAdaptor_Curve curve2(edge2);

    if (curve1.GetType() == GeomAbs_Line && curve2.GetType() == GeomAbs_Line) {
        gp_Lin line1 = curve1.Line();
        gp_Lin line2 = curve2.Line();

        gp_Dir dir1 = line1.Direction();
        gp_Dir dir2 = line2.Direction();

        // Check if lines are parallel
        if (dir1.IsParallel(dir2, Precision::Angular())) {
            return true;
        }
    }

    return false;
}

unsigned int Measurement::getMemSize() const
{
    return 0;
}

PyObject* Measurement::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new MeasurementPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
