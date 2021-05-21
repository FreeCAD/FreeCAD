/***************************************************************************
 *   Copyright (c) 2013      Luke Parry <l.parry@warwick.ac.uk>            *
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
  # include <BRep_Builder.hxx>
  # include <TopoDS_Compound.hxx>
  # include <TopoDS_Shape.hxx>
  # include <TopoDS_Face.hxx>
  # include <TopoDS.hxx>
  # include <TopExp_Explorer.hxx>
  # include <gp_Pln.hxx>
  # include <gp_Ax3.hxx>
  # include <gp_Circ.hxx>
  # include <gp_Elips.hxx>
  # include <GCPnts_AbscissaPoint.hxx>
  # include <BRepAdaptor_Surface.hxx>
  # include <BRepAdaptor_Curve.hxx>
  # include <BRepExtrema_DistShapeShape.hxx>
  # include <GProp_GProps.hxx>
  # include <GeomAPI_ExtremaCurveCurve.hxx>
  # include <BRepGProp.hxx>
  # include <ShapeAnalysis_Surface.hxx>
  # include <Geom_Surface.hxx>
#endif


#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/VectorPy.h>

#include <App/DocumentObserver.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include "Measurement.h"
#include "MeasurementPy.h"

#ifndef M_PI
    #define M_PI    3.14159265358979323846 /* pi */
#endif
using namespace Measure;
using namespace Base;
using namespace Part;

typedef std::unordered_set<TopoDS_Shape,Part::ShapeHasher,Part::ShapeHasher> ShapeSet;
typedef std::unordered_map<TopoDS_Shape,int,Part::ShapeHasher,Part::ShapeHasher> ShapeMap;

FC_LOG_LEVEL_INIT("Measure");

TYPESYSTEM_SOURCE(Measure::Measurement, Base::BaseClass)

Measurement::Measurement()
{
    measureType = Invalid;
    References3D.setScope(App::LinkScope::Global);
}

Measurement::~Measurement()
{

}

#define _MEASURE_ERROR(_err, _msg) do {\
    if (this->silent) \
        FC_ERR(_msg);\
    else \
        FC_THROWM(_err,_msg);\
} while(0)

#define MEASURE_ERROR(_msg) _MEASURE_ERROR(Base::RuntimeError, _msg)
        
void Measurement::clear()
{
    std::vector<App::DocumentObject*> Objects;
    std::vector<std::string> SubElements;
    References3D.setValues(Objects, SubElements);
    measureType = Invalid;
}

bool Measurement::has3DReferences()
{
    return (References3D.getSize() > 0);
}

//add a 3D reference (obj+sub) to end of list
int Measurement::addReference3D(App::DocumentObject *obj, const std::string& subName)
{
    return addReference3D(obj,subName.c_str());
}

///add a 3D reference (obj+sub) to end of list
int Measurement::addReference3D(App::DocumentObject *obj, const char* subName)
{
  std::vector<App::DocumentObject*> objects = References3D.getValues();
  std::vector<std::string> subElements = References3D.getSubValues();

  objects.push_back(obj);
  subElements.push_back(subName);

  References3D.setValues(objects, subElements);

  measureType = getType();
  return References3D.getSize();
}

MeasureType Measurement::getType()
{
    const std::vector<App::DocumentObject*> &objects = References3D.getValues();
    const std::vector<std::string> &subElements = References3D.getSubValues();

    std::vector<App::DocumentObject*>::const_iterator obj = objects.begin();
    std::vector<std::string>::const_iterator subEl = subElements.begin();

    //
    //int dims = -1;
    MeasureType mode;

    // Type of References3D
    int verts = 0;
    int edges = 0;
    int faces = 0;
    int vols = 0;

    for (;obj != objects.end(); ++obj, ++subEl) {

        // Check if solid object
        if(strcmp((*subEl).c_str(), "") == 0) {
            vols++;
        } else {

            TopoDS_Shape refSubShape;
            try {
                refSubShape = Part::Feature::getShape(*obj,(*subEl).c_str(),true);
                if(refSubShape.IsNull())
                    return Invalid;
            }
            catch (Standard_Failure& e) {
                _MEASURE_ERROR(Base::CADKernelError,
                        "Measurement::getType - " << e.GetMessageString());
                return Invalid;
            }

            switch (refSubShape.ShapeType()) {
              case TopAbs_VERTEX:
                {
                    verts++;
                }
                break;
              case TopAbs_EDGE:
                {
                    edges++;
                }
                break;
              case TopAbs_FACE:
                {
                    faces++;
                }
                break;
              default:
                break;
            }
        }
    }

    if(vols > 0) {
        if(verts > 0 || edges > 0 || faces > 0) {
            mode = Invalid;
        } else {
            mode = Volumes;
        }
    } else if(faces > 0) {
        if(verts > 0 || edges > 0) {
            if((faces > 1 && verts > 1) || edges > 0) {
                mode = Invalid;
            } else {
                // One Surface and One Point
                mode = PointToSurface;
            }
        } else {
            mode = Surfaces;
        }
    } else if(edges > 0) {
        if(verts > 0) {
            if(verts > 1 && edges > 0) {
                mode = Invalid;
            } else {
                mode = PointToEdge;
            }
        } else {
//             if(edges == 2) {
//                 mode = EdgeToEdge;
//             } else {
                mode = Edges;
//             }
        }
    } else if (verts > 0) {
        mode = Points;
    } else {
        mode = Invalid;
    }

    return mode;
}

TopoDS_Shape Measurement::getShape(App::DocumentObject *obj , const char *subName) const
{
    try {
        auto shape = Part::Feature::getShape(obj,subName,true);
        if (shape.IsNull())
            _MEASURE_ERROR(Part::NullShapeException,
                    "Measurement::getShape - null shape");
        return shape;
    } catch (Standard_Failure& e) {
        _MEASURE_ERROR(Part::NullShapeException,
                "Measurement::getShape - " << e.GetMessageString());
        return TopoDS_Shape();
    }
}

double Measurement::length(const TopoDS_Shape &shape) const
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) {
        MEASURE_ERROR("Measurement::length - not an edge");
        return -1.0;
    }
    const TopoDS_Edge& edge = TopoDS::Edge(shape);
    BRepAdaptor_Curve curve(edge);
    switch(curve.GetType()) {
    case GeomAbs_Line : {
        gp_Pnt P1 = curve.Value(curve.FirstParameter());
        gp_Pnt P2 = curve.Value(curve.LastParameter());
        gp_XYZ diff = P2.XYZ() - P1.XYZ();
        return diff.Modulus();
    }
    case GeomAbs_Circle : {
        double u = curve.FirstParameter();
        double v = curve.LastParameter();
        double radius = curve.Circle().Radius();
        if (u > v) // if arc is reversed
            std::swap(u, v);

        double range = v-u;
        return radius * range;
    }
    default:
        try {
            return GCPnts_AbscissaPoint::Length(curve);
        } catch (Standard_Failure &e) {
            MEASURE_ERROR("Measurement::length - failed - " << e.GetMessageString());
            return -1.0;
        }
    }
    return -1.0;
}

//TODO:: add lengthX, lengthY (and lengthZ??) support
// Methods for distances (edge length, two points, edge and a point
double Measurement::length() const
{
    int numRefs = References3D.getSize();
    if(numRefs == 0) {
        MEASURE_ERROR("Measurement::length - No 3D references available");
        return 0.0;
    }

    if(measureType == Points ||
        measureType == PointToEdge ||
        measureType == PointToSurface)  {

        Base::Vector3d diff = this->delta();
        return diff.Length();
    }

    std::map<App::SubObjectT, TopoShape> shapemap;
    ShapeSet shapeset;
    double result = 0.0;
    const auto &objs = References3D.getValues();
    const auto &subs = References3D.getSubValues();
    for (int i=0; i<numRefs; ++i) {
        App::SubObjectT key(objs[i], subs[i].c_str());
        std::string element = key.getElementName();
        // Strip off any sub-element name, so that we can key on the same
        // sub-object and cache its whole shape. This is necessary for filtering
        // out same sub-element in the references, because
        // Part::Feature::getTopoShape() will calculate the accumulated
        // transformation along the way and return a shape that is not suitable
        // for shape search using OCC shape hash (which hashes on pointers to
        // TShape and TopLoc_Location).
        key.setSubName(key.getSubNameNoElement());
        auto &shape = shapemap[key];
        if (shape.isNull())
            shape = Part::Feature::getTopoShape(objs[i], key.getSubName().c_str());
        auto subshape = shape.getSubTopoShape(element.c_str());
        // Skip duplicated sub shape references, but DO NOT skip duplicated
        // edges. Use 'perimeter' for that.
        if (!shapeset.insert(subshape.getShape()).second)
            continue;
        for (auto & edge : subshape.getSubShapes(TopAbs_EDGE)) {
            double l = length(edge);
            if (l < 0.0)
                return 0.0;
            result += l;
        }
    }  //end for
    return result;
}

double Measurement::perimeter(bool checkInner) const
{
    std::map<App::SubObjectT, TopoShape> shapemap;
    ShapeMap shapecounter;
    ShapeSet shapeset;
    const auto &objs = References3D.getValues();
    const auto &subs = References3D.getSubValues();
    int count = 0;
    std::size_t inner = 0;
    double result = 0.0;
    std::vector<Part::TopoShape> innerWires;
    for (std::size_t i=0; i<objs.size(); ++i) {
        App::SubObjectT key(objs[i], subs[i].c_str());
        std::string element = key.getElementName();
        key.setSubName(key.getSubNameNoElement());
        auto &shape = shapemap[key];
        if (shape.isNull())
            shape = Part::Feature::getTopoShape(objs[i], key.getSubName().c_str());
        auto subshape = shape.getSubTopoShape(element.c_str());
        for (auto & face : subshape.getSubTopoShapes(TopAbs_FACE)) {
            if (!shapeset.insert(face.getShape()).second)
                continue; // skip duplicated face reference
            innerWires.clear();
            auto wire = face.splitWires(&innerWires, Part::TopoShape::NoReorient);
            inner += innerWires.size();
            count += wire.countSubShapes(TopAbs_EDGE);
            for (auto & edge : wire.getSubShapes(TopAbs_EDGE)) {
                auto &counter  = shapecounter[edge];
                if (counter > 1)
                    continue;
                double l = length(edge);
                if (l < 0.0)
                    return 0.0;
                if (++counter == 2) {
                    // if edge is included more than once, it means that it is
                    // shared by more than one face, and hence is considered as
                    // an internal edge.
                    ++inner;
                    result -= l;
                } else
                    result += l;
            }
        }
    }
    if (checkInner && !inner)
        MEASURE_ERROR("Measurement::perimeter -- no inner wires");
    if (!count)
        MEASURE_ERROR("Measurement::perimeter -- no edge found");
    return result;
}

double Measurement::area() const
{
    std::map<App::SubObjectT, TopoShape> shapemap;
    ShapeSet shapeset;
    const auto &objs = References3D.getValues();
    const auto &subs = References3D.getSubValues();
    double result = 0.0;
    int count = 0;
    try {
        for (std::size_t i=0; i<objs.size(); ++i) {
            App::SubObjectT key(objs[i], subs[i].c_str());
            std::string element = key.getElementName();
            key.setSubName(key.getSubNameNoElement());
            auto &shape = shapemap[key];
            if (shape.isNull())
                shape = Part::Feature::getTopoShape(objs[i], key.getSubName().c_str());
            auto subshape = shape.getSubTopoShape(element.c_str());
            for (auto & face : subshape.getSubShapes(TopAbs_FACE)) {
                if (!shapeset.insert(face).second)
                    continue;
                ++count;
                GProp_GProps props;
                BRepGProp::SurfaceProperties(face, props);
                result += props.Mass();
            }
        }
    } catch (Standard_Failure &e) {
        MEASURE_ERROR("Measurement::area -- " << e.GetMessageString());
        return 0.0;
    }
    if (!count)
        MEASURE_ERROR("Measurement::area -- no face found");
    return result;
}

double Measurement::volume() const
{
    std::map<App::SubObjectT, TopoShape> shapemap;
    ShapeSet shapeset;
    const auto &objs = References3D.getValues();
    const auto &subs = References3D.getSubValues();
    double result = 0.0;
    int count = 0;
    try {
        for (std::size_t i=0; i<objs.size(); ++i) {
            App::SubObjectT key(objs[i], subs[i].c_str());
            std::string element = key.getElementName();
            key.setSubName(key.getSubNameNoElement());
            auto &shape = shapemap[key];
            if (shape.isNull())
                shape = Part::Feature::getTopoShape(objs[i], key.getSubName().c_str());
            auto subshape = shape.getSubTopoShape(element.c_str());
            for (auto & shell : subshape.getSubShapes(TopAbs_SHELL)) {
                if (!shapeset.insert(shell).second)
                    continue;
                ++count;
                GProp_GProps props;
                BRepGProp::VolumeProperties(shell, props);
                result += props.Mass();
            }
        }
    } catch (Standard_Failure &e) {
        MEASURE_ERROR("Measurement::volume -- " << e.GetMessageString());
        return 0.0;
    }
    if (!count)
        MEASURE_ERROR("Measurement::volume -- no shell found");
    return result;
}

double Measurement::angle(const Base::Vector3d & /*param*/) const
{
    int numRefs = References3D.getSize();
    if(numRefs <= 1) {
        MEASURE_ERROR("Measurement::angle - Not enough references available");
        return 0.0;
    }

    gp_Dir dir1, dir2;
    const std::vector<App::DocumentObject*> &objects = References3D.getValues();
    const std::vector<std::string> &subElements = References3D.getSubValues();

    TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
    TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());
    TopoDS_Shape shape3;
    if (shape1.IsNull() || shape2.IsNull())
        return 0.0;

    if (shape1.ShapeType() == TopAbs_VERTEX
            || shape2.ShapeType() == TopAbs_VERTEX)
    {
        if(numRefs != 3) {
            MEASURE_ERROR("Measurement::angle - Not enough references available");
            return 0.0;
        }
        TopoDS_Shape shape3 = getShape(objects.at(2), subElements.at(2).c_str());
        if (shape3.IsNull()
                || shape1.ShapeType() != TopAbs_VERTEX
                || shape2.ShapeType() != TopAbs_VERTEX
                || shape3.ShapeType() != TopAbs_VERTEX)
        {
            MEASURE_ERROR("Measurement::angle - wrong type of reference");
            return 0.0;
        }
        gp_Pnt p1 = BRep_Tool::Pnt(TopoDS::Vertex(shape1));
        gp_Pnt p2 = BRep_Tool::Pnt(TopoDS::Vertex(shape2));
        gp_Pnt p3 = BRep_Tool::Pnt(TopoDS::Vertex(shape3));
        dir1 = gp_Dir(p1.XYZ() - p2.XYZ());
        dir2 = gp_Dir(p2.XYZ() - p3.XYZ());
    } else if (numRefs != 2) {
        MEASURE_ERROR("Measurement::angle - Too many references");
        return 0.0;
    } else if ((shape1.ShapeType() != TopAbs_EDGE && shape1.ShapeType() != TopAbs_FACE)
            || (shape2.ShapeType() != TopAbs_EDGE && shape2.ShapeType() != TopAbs_FACE))
    {
        MEASURE_ERROR("Measurement::angle - Wrong type of references");
        return 0.0;
    } else {
        std::unique_ptr<Part::Geometry> geo1(Part::Geometry::fromShape(shape1, this->silent));
        std::unique_ptr<Part::Geometry> geo2(Part::Geometry::fromShape(shape2, this->silent));
        if (!geo1 || !geo2) {
            MEASURE_ERROR("Measurement::angle - failed to obtain geometry from shape");
            return 0.0;
        }
        int intersected = -1;
        gp_Pnt p1, p2;
        auto checkIntersection = [&]() {
            if (intersected >= 0)
                return intersected;
            try {
                intersected = 0;
                BRepExtrema_DistShapeShape extrema(shape1, shape2);
                if(extrema.IsDone()) {
                    // Found the nearest point between two shapes NOTE we will
                    // assume there is only 1 solution (cyclic topology will
                    // create multiple solutions.
                    p1 = extrema.PointOnShape1(1);
                    p2 = extrema.PointOnShape2(1);
                    intersected = 1;
                }
            } catch (Standard_Failure& e) {
                MEASURE_ERROR("Measurement::angle - checkIntersection failed - "
                        << e.GetMessageString());
            }
            return intersected;
        };
        auto getDirection = [&] (gp_Dir &dir,
                                 gp_Pnt &pnt,
                                 Part::Geometry *geo) -> bool
        {
            if (auto g = Base::freecad_dynamic_cast<Part::GeomCurve>(geo)) {
                if (g->isLinear()) {
                    auto v = g->pointAtParameter(g->getLastParameter());
                    v -= g->pointAtParameter(g->getFirstParameter());
                    dir = gp_Dir(v.x, v.y, v.z);
                    return true;
                }
                if (!checkIntersection())
                    return false;
                double u;
                if (!g->closestParameterToBasisCurve(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()), u))
                    return false;
                return g->tangent(u, dir);
            } else if (auto g = Base::freecad_dynamic_cast<Part::GeomSurface>(geo)) {
                if (g->isPlanar()) {
                    std::unique_ptr<Part::GeomPlane> plane(g->toPlane());
                    return plane->normal(0, 0, dir);
                }
                if (!checkIntersection())
                    return false;
                double u, v;
                Handle(Geom_Surface) surf = Handle(Geom_Surface)
                    ::DownCast(g->handle());
                try {
                    ShapeAnalysis_Surface as(surf);
                    gp_Pnt2d uv = as.ValueOfUV(pnt, 1e-7);
                    u = uv.X();
                    v = uv.Y();
                } catch (Standard_Failure &e) {
                    MEASURE_ERROR("Measurement::angle - failed to get surface parameter "
                            << e.GetMessageString());
                    return false;
                }
                return g->normal(u, v, dir);
            }
            MEASURE_ERROR("Measurement::angle - unknown geometry type");
            return false;
        };
        if (!getDirection(dir1, p1, geo1.get()) || !getDirection(dir2, p2, geo2.get()))
            return 0.0;
    }
    Standard_Real aRad = dir1.Angle(dir2);
    return aRad * 180  / M_PI;
}

double Measurement::radius() const
{
    int numRefs = References3D.getSize();
    if(numRefs == 0) {
        MEASURE_ERROR("Measurement::radius - No References3D provided");
        return 0.0;
    } else {
        if(numRefs == 1 || measureType == Edges) {
            const std::vector<App::DocumentObject*> &objects = References3D.getValues();
            const std::vector<std::string> &subElements = References3D.getSubValues();

            TopoDS_Shape shape = getShape(objects.at(0), subElements.at(0).c_str());
            if (shape.IsNull())
                return 0.0;
            const TopoDS_Edge& edge = TopoDS::Edge(shape);

            BRepAdaptor_Curve curve(edge);
            if(curve.GetType() == GeomAbs_Circle) {
                return (double) curve.Circle().Radius();
            }
        }
    }
    MEASURE_ERROR("Measurement::radius - Invalid References3D Provided");
    return 0.0;
}

Base::Vector3d Measurement::delta() const
{
    int numRefs =  References3D.getSize();
    if (numRefs == 0) {
        MEASURE_ERROR("Measurement::delta - No 3D references available");
        return Base::Vector3d();
    } else if (measureType == Invalid) {
        MEASURE_ERROR("Measurement::delta - measureType is Invalid");
        return Base::Vector3d();
    } else {
        const std::vector<App::DocumentObject*> &objects = References3D.getValues();
        const std::vector<std::string> &subElements = References3D.getSubValues();

        if(measureType == Points) {
            if(numRefs == 2) {
                // Keep separate case for two points to reduce need for complex algorithm
                TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());
                if (shape1.IsNull() || shape2.IsNull())
                    return Base::Vector3d();

                const TopoDS_Vertex& vert1 = TopoDS::Vertex(shape1);
                const TopoDS_Vertex& vert2 = TopoDS::Vertex(shape2);

                gp_Pnt P1 = BRep_Tool::Pnt(vert1);
                gp_Pnt P2 = BRep_Tool::Pnt(vert2);
                gp_XYZ diff = P2.XYZ() - P1.XYZ();
                return Base::Vector3d(diff.X(), diff.Y(), diff.Z());
            }
        } else if(measureType == PointToEdge ||
                  measureType == PointToSurface) {
            // BrepExtema can calculate minimum distance between any set of topology sets.
            if(numRefs == 2) {
                TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());
                if (shape1.IsNull() || shape2.IsNull())
                    return Base::Vector3d();

                BRepExtrema_DistShapeShape extrema(shape1, shape2);

                if(extrema.IsDone()) {
                    // Found the nearest point between point and curve
                    // NOTE we will assume there is only 1 solution (cyclic topology will create multiple solutions.
                    gp_Pnt P1 = extrema.PointOnShape1(1);
                    gp_Pnt P2 = extrema.PointOnShape2(1);
                    gp_XYZ diff = P2.XYZ() - P1.XYZ();
                    return Base::Vector3d(diff.X(), diff.Y(), diff.Z());
                }
            }
        } else if(measureType == Edges) {
            // Only case that is supported is straight line edge
            if(numRefs == 1) {
                TopoDS_Shape shape = getShape(objects.at(0), subElements.at(0).c_str());
                if (shape.IsNull())
                    return Base::Vector3d();
                const TopoDS_Edge& edge = TopoDS::Edge(shape);
                BRepAdaptor_Curve curve(edge);

                if(curve.GetType() == GeomAbs_Line) {
                      gp_Pnt P1 = curve.Value(curve.FirstParameter());
                      gp_Pnt P2 = curve.Value(curve.LastParameter());
                      gp_XYZ diff = P2.XYZ() - P1.XYZ();
                      return Base::Vector3d(diff.X(), diff.Y(), diff.Z());
                }
            } else if(numRefs == 2) {
                TopoDS_Shape shape1 = getShape(objects.at(0), subElements.at(0).c_str());
                TopoDS_Shape shape2 = getShape(objects.at(1), subElements.at(1).c_str());
                if (shape1.IsNull() || shape2.IsNull())
                    return Base::Vector3d();

                BRepAdaptor_Curve curve1(TopoDS::Edge(shape1));
                BRepAdaptor_Curve curve2(TopoDS::Edge(shape2));

                // Only permit line to line distance
                if(curve1.GetType() == GeomAbs_Line &&
                   curve2.GetType() == GeomAbs_Line) {
                    BRepExtrema_DistShapeShape extrema(shape1, shape2);

                    if(extrema.IsDone()) {
                        // Found the nearest point between point and curve
                        // NOTE we will assume there is only 1 solution (cyclic topology will create multiple solutions.
                        gp_Pnt P1 = extrema.PointOnShape1(1);
                        gp_Pnt P2 = extrema.PointOnShape2(1);
                        gp_XYZ diff = P2.XYZ() - P1.XYZ();
                        return Base::Vector3d(diff.X(), diff.Y(), diff.Z());
                    }
                }
            }
        } else {
            MEASURE_ERROR("Measurement::delta - measureType is not recognized");
            return Base::Vector3d();
        }
    }
    MEASURE_ERROR("Measurement::delta - failed");
    return Base::Vector3d();
}


Base::Vector3d Measurement::massCenter() const
{
    Base::Vector3d result;
    int numRefs =  References3D.getSize();
    if (numRefs == 0) {
        MEASURE_ERROR("Measurement::massCenter - No 3D references available");
    } else if (measureType == Invalid) {
        MEASURE_ERROR("Measurement::massCenter - measureType is Invalid");
    } else {
        const std::vector<App::DocumentObject*> &objects = References3D.getValues();
        const std::vector<std::string> &subElements = References3D.getSubValues();
        GProp_GProps gprops = GProp_GProps();

        if(measureType == Volumes) {
            // Iterate through edges and calculate each length
            std::vector<App::DocumentObject*>::const_iterator obj = objects.begin();
            std::vector<std::string>::const_iterator subEl = subElements.begin();

            for (;obj != objects.end(); ++obj, ++subEl) {
                //const Part::Feature *refObj = static_cast<const Part::Feature*>((*obj));
                //const Part::TopoShape& refShape = refObj->Shape.getShape();

                // Compute inertia properties

                GProp_GProps props = GProp_GProps();
                auto shape = getShape((*obj), "");
                if (shape.IsNull())
                    return result;
                BRepGProp::VolumeProperties(shape, props);
                gprops.Add(props);
                // Get inertia properties
            }

            //double mass = gprops.Mass();
            gp_Pnt cog = gprops.CentreOfMass();

            return Base::Vector3d(cog.X(), cog.Y(), cog.Z());
        } else {
            MEASURE_ERROR("Measurement::massCenter - measureType is not recognized");
        }
    }
    return result;
}

unsigned int Measurement::getMemSize(void) const
{
    return 0;
}

PyObject *Measurement::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new MeasurementPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
