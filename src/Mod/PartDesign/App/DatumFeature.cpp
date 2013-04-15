/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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
# include <cfloat>
# include <BRepLib.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_GTransform.hxx>
# include <gp_GTrsf.hxx>
# include <Geom_Plane.hxx>
# include <Geom2d_Line.hxx>
# include <Handle_Geom_Plane.hxx>
# include <Handle_Geom2d_Line.hxx>
# include <Precision.hxx>
# include <Standard_Real.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Vertex.hxx>
# include <Standard_Version.hxx>
#endif


#include <App/Plane.h>
#include "DatumFeature.h"
#include <Base/Tools.h>
#include <Base/Exception.h>
#include "Mod/Part/App/PrimitiveFeature.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;

PROPERTY_SOURCE_ABSTRACT(PartDesign::Datum, PartDesign::Feature)

Datum::Datum(void)
{
    ADD_PROPERTY_TYPE(References,(0,0),"References",(App::PropertyType)(App::Prop_None),"References defining the datum feature");
    ADD_PROPERTY(Values,(0.0));
    touch();
}

Datum::~Datum()
{
}

short Datum::mustExecute(void) const
{
    if (References.isTouched() ||
        Values.isTouched())
        return 1;
    return Feature::mustExecute();
}

void Datum::onChanged(const App::Property* prop)
{

    if (prop == &References) {
        refTypes.clear();
        std::vector<App::DocumentObject*> refs = References.getValues();
        std::vector<std::string> refnames = References.getSubValues();

        for (int r = 0; r < refs.size(); r++)
            refTypes.insert(getRefType(refs[r], refnames[r]));
    }

    PartDesign::Feature::onChanged(prop);
}

// Note: We don't distinguish between e.g. datum lines and edges here
// These values are just markers so it doesn't matter that they are Part features
#define PLANE Part::Plane::getClassTypeId()
#define LINE  Part::Line::getClassTypeId()
#define POINT Part::Vertex::getClassTypeId()

const Base::Type Datum::getRefType(const App::DocumentObject* obj, const std::string& subname)
{
    Base::Type type = obj->getTypeId();

    if ((type == App::Plane::getClassTypeId()) || (type == PartDesign::Plane::getClassTypeId()))
        return PLANE;
    else if (type == PartDesign::Line::getClassTypeId())
        return LINE;
    else if (type == PartDesign::Point::getClassTypeId())
        return POINT;
    else if (type.isDerivedFrom(Part::Feature::getClassTypeId())) {
        // Note: For now, only planar references are possible
        if (subname.size() > 4 && subname.substr(0,4) == "Face")
            return PLANE;
        else if (subname.size() > 4 && subname.substr(0,4) == "Edge")
            return LINE;
        else if (subname.size() > 6 && subname.substr(0,6) == "Vertex")
            return POINT;
    }

    throw Base::Exception("PartDesign::Datum::getRefType(): Illegal object type");
}

// ================================ Initialize the hints =====================

std::map<std::multiset<Base::Type>, std::set<Base::Type> > Point::hints = std::map<std::multiset<Base::Type>, std::set<Base::Type> >();

void Point::initHints()
{
    std::set<Base::Type> DONE;
    DONE.insert(PartDesign::Point::getClassTypeId());

    std::multiset<Base::Type> key;
    std::set<Base::Type> value;
    key.insert(POINT);
    hints[key] = DONE; // POINT -> DONE. Point from another point or vertex

    key.clear(); value.clear();
    key.insert(LINE);
    value.insert(LINE); value.insert(PLANE);
    hints[key] = value; // LINE -> LINE or PLANE

    key.clear(); value.clear();
    key.insert(LINE); key.insert(LINE);
    hints[key] = DONE; // {LINE, LINE} -> DONE. Point from two lines or edges

    key.clear(); value.clear();
    key.insert(LINE); key.insert(PLANE);
    hints[key] = DONE; // {LINE, PLANE} -> DONE. Point from line and plane

    key.clear(); value.clear();
    key.insert(PLANE);
    value.insert(PLANE); value.insert(LINE);
    hints[key] = value; // PLANE -> PLANE or LINE

    key.clear(); value.clear();
    key.insert(PLANE); key.insert(PLANE);
    value.insert(PLANE);
    hints[key] = value; // {PLANE, PLANE} -> PLANE

    key.clear(); value.clear();
    key.insert(PLANE); key.insert(PLANE); key.insert(PLANE);
    hints[key] = DONE; // {PLANE, PLANE, PLANE} -> DONE. Point from three planes

    key.clear(); value.clear();
    value.insert(POINT); value.insert(LINE); value.insert(PLANE);
    hints[key] = value;
}

std::map<std::multiset<Base::Type>, std::set<Base::Type> > Line::hints = std::map<std::multiset<Base::Type>, std::set<Base::Type> >();

void Line::initHints()
{
    std::set<Base::Type> DONE;
    DONE.insert(PartDesign::Line::getClassTypeId());

    std::multiset<Base::Type> key;
    std::set<Base::Type> value;
    key.insert(LINE);
    hints[key] = DONE; // LINE -> DONE. Line from another line or edge

    key.clear(); value.clear();
    key.insert(POINT);
    value.insert(POINT);
    hints[key] = value; // POINT -> POINT

    key.clear(); value.clear();
    key.insert(POINT); key.insert(POINT);
    hints[key] = DONE; // {POINT, POINT} -> DONE. Line from two points or vertices

    key.clear(); value.clear();
    key.insert(PLANE);
    value.insert(PLANE);
    hints[key] = value; // PLANE -> PLANE

    key.clear(); value.clear();
    key.insert(PLANE); key.insert(PLANE);
    hints[key] = DONE; // {PLANE, PLANE} -> DONE. Line from two planes or faces

    key.clear(); value.clear();
    value.insert(POINT); value.insert(LINE); value.insert(PLANE);
    hints[key] = value;
}

std::map<std::multiset<Base::Type>, std::set<Base::Type> > Plane::hints = std::map<std::multiset<Base::Type>, std::set<Base::Type> >();

void Plane::initHints()
{
    std::set<Base::Type> DONE;
    DONE.insert(PartDesign::Plane::getClassTypeId());

    std::multiset<Base::Type> key;
    std::set<Base::Type> value;
    key.insert(PLANE);
    hints[key] = DONE; // PLANE -> DONE. Plane from another plane or face

    key.clear(); value.clear();
    key.insert(POINT);
    value.insert(POINT); value.insert(LINE);
    hints[key] = value; // POINT -> POINT or LINE

    key.clear(); value.clear();
    key.insert(POINT); key.insert(LINE);
    hints[key] = DONE; // {POINT, LINE} -> DONE. Plane from point/vertex and line/edge

    key.clear(); value.clear();
    key.insert(POINT); key.insert(POINT);
    value.insert(POINT);
    hints[key] = value; // {POINT, POINT} -> POINT

    key.clear(); value.clear();
    key.insert(POINT); key.insert(POINT); key.insert(POINT);
    hints[key] = DONE; // {POINT, POINT, POINT} -> DONE. Plane from 3 points or vertices

    key.clear(); value.clear();
    key.insert(LINE);
    value.insert(POINT);
    hints[key] = value; // LINE -> POINT

    key.clear(); value.clear();
    value.insert(POINT); value.insert(LINE); value.insert(PLANE);
    hints[key] = value;
}

// ============================================================================

PROPERTY_SOURCE(PartDesign::Point, PartDesign::Datum)

Point::Point()
{    
}

Point::~Point()
{
}

short Point::mustExecute() const
{
    return PartDesign::Datum::mustExecute();
}

App::DocumentObjectExecReturn *Point::execute(void)
{
    std::set<Base::Type> hint = getHint();
    if (!((hint.size() == 1) && (hint.find(PartDesign::Point::getClassTypeId()) != hint.end())))
        return App::DocumentObject::StdReturn; // incomplete references

    // Extract the shapes of the references
    std::vector<TopoDS_Shape> shapes;
    std::vector<App::DocumentObject*> refs = References.getValues();
    std::vector<std::string> refnames = References.getSubValues();
    for (int i = 0; i < refs.size(); i++) {
        if (!refs[i]->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
            return new App::DocumentObjectExecReturn("PartDesign::Point: Invalid reference type");
        Part::Feature* feature = static_cast<Part::Feature*>(refs[i]);
        const TopoDS_Shape& sh = feature->Shape.getValue();
        if (sh.IsNull())
            return new App::DocumentObjectExecReturn("PartDesign::Point: Reference has NULL shape");
        if (refnames[i].empty()) {
            // Datum feature or App::Plane
            shapes.push_back(sh);
        } else {
            // Get subshape
            TopoDS_Shape subshape = feature->Shape.getShape().getSubShape(refnames[i].c_str());
            if (subshape.IsNull())
                return new App::DocumentObjectExecReturn("PartDesign::Point: Reference has NULL subshape");
            shapes.push_back(subshape);
        }
    }

    // Find the point
    gp_Pnt point(0,0,0);

    if (shapes.size() == 1) {
        // Point from vertex or other point
        if (shapes[0].ShapeType() != TopAbs_VERTEX)
            return new App::DocumentObjectExecReturn("PartDesign::Point::execute(): Internal error, unexpected ShapeType");
        TopoDS_Vertex v = TopoDS::Vertex(shapes[0]);
        //point.X = v.
    }
    
    BRepBuilderAPI_MakeVertex MakeVertex(point);
    const TopoDS_Vertex& vertex = MakeVertex.Vertex();
    this->Shape.setValue(vertex);

    return App::DocumentObject::StdReturn;
}


const std::set<Base::Type> Point::getHint()
{
    if (hints.find(refTypes) != hints.end())
        return hints[refTypes];
    else
        return std::set<Base::Type>();
}


PROPERTY_SOURCE(PartDesign::Line, PartDesign::Datum)

Line::Line()
{
}

Line::~Line()
{
}

short Line::mustExecute() const
{
    return PartDesign::Datum::mustExecute();
}

App::DocumentObjectExecReturn *Line::execute(void)
{
    gp_Pnt point1(0,0,0);

    gp_Pnt point2(10,10,10);

    BRepBuilderAPI_MakeEdge mkEdge(point1, point2);
    if (!mkEdge.IsDone())
        return new App::DocumentObjectExecReturn("Failed to create edge");
    const TopoDS_Edge& edge = mkEdge.Edge();
    this->Shape.setValue(edge);

    return App::DocumentObject::StdReturn;
}


const std::set<Base::Type> Line::getHint()
{
    if (hints.find(refTypes) != hints.end())
        return hints[refTypes];
    else
        return std::set<Base::Type>();
}


PROPERTY_SOURCE(PartDesign::Plane, PartDesign::Datum)

Plane::Plane()
{
}

short Plane::mustExecute() const
{
    return PartDesign::Datum::mustExecute();
}

App::DocumentObjectExecReturn *Plane::execute(void)
{
    double O = 10.0; //this->Offset.getValue();
    double A = 45.0; //this->Angle.getValue();

    if (fabs(A) > 360.0)
      return new App::DocumentObjectExecReturn("Angle too large (please use -360.0 .. +360.0)");

    gp_Pnt pnt(0.0,0.0,0.0);
    gp_Dir dir(0.0,0.0,1.0);
    Handle_Geom_Plane aPlane = new Geom_Plane(pnt, dir);
    BRepBuilderAPI_MakeFace mkFace(aPlane, 0.0, 100.0, 0.0, 100.0
#if OCC_VERSION_HEX >= 0x060502
      , Precision::Confusion()
#endif
    );

    TopoDS_Shape ResultShape = mkFace.Shape();
    this->Shape.setValue(ResultShape);

    return App::DocumentObject::StdReturn;
}


const std::set<Base::Type> Plane::getHint()
{
    if (hints.find(refTypes) != hints.end())
        return hints[refTypes];
    else
        return std::set<Base::Type>();
}
