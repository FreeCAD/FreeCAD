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
# include <BRep_Tool.hxx>
# include <gp_GTrsf.hxx>
# include <Geom_Plane.hxx>
# include <Geom2d_Line.hxx>
# include <Handle_Geom_Curve.hxx>
# include <Handle_Geom_Surface.hxx>
# include <Handle_Geom_Plane.hxx>
# include <Handle_Geom2d_Line.hxx>
# include <GeomAPI_IntCS.hxx>
# include <GeomAPI_IntSS.hxx>
# include <GeomAPI_ExtremaCurveCurve.hxx>
# include <Precision.hxx>
# include <Standard_Real.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <Standard_Version.hxx>
#endif

#include <QObject>
#include <App/Plane.h>
#include "DatumFeature.h"
#include <Base/Tools.h>
#include <Base/Exception.h>
#include <Base/Console.h>
#include "Mod/Part/App/PrimitiveFeature.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;

PROPERTY_SOURCE_ABSTRACT(PartDesign::Datum, App::DocumentObject)

Datum::Datum(void)
{
    ADD_PROPERTY_TYPE(References,(0,0),"References",(App::PropertyType)(App::Prop_None),"References defining the datum feature");
    ADD_PROPERTY(Values,(0.0));
    touch();
}

Datum::~Datum()
{
}


App::DocumentObjectExecReturn *Datum::execute(void)
{
    References.touch();
    return StdReturn;
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

    App::DocumentObject::onChanged(prop);
}

void Datum::onDocumentRestored()
{
    // This seems to be the only way to make the ViewProvider display the datum feature
    References.touch();
    App::DocumentObject::onDocumentRestored();
}

// Note: We don't distinguish between e.g. datum lines and edges here
#define PLANE QObject::tr("DPLANE")
#define LINE  QObject::tr("DLINE")
#define POINT QObject::tr("DPOINT")

const QString Datum::getRefType(const App::DocumentObject* obj, const std::string& subname)
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

std::map<std::multiset<QString>, std::set<QString> > Point::hints = std::map<std::multiset<QString>, std::set<QString> >();

void Point::initHints()
{
    std::set<QString> DONE;
    DONE.insert(QObject::tr("Point"));

    std::multiset<QString> key;
    std::set<QString> value;
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

std::map<std::multiset<QString>, std::set<QString> > Line::hints = std::map<std::multiset<QString>, std::set<QString> >();

void Line::initHints()
{
    std::set<QString> DONE;
    DONE.insert(QObject::tr("Line"));

    std::multiset<QString> key;
    std::set<QString> value;
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

std::map<std::multiset<QString>, std::set<QString> > Plane::hints = std::map<std::multiset<QString>, std::set<QString> >();

void Plane::initHints()
{
    std::set<QString> DONE;
    DONE.insert(QObject::tr("Plane"));

    std::multiset<QString> key;
    std::set<QString> value;
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
    ADD_PROPERTY_TYPE(_Point,(Base::Vector3d(0,0,1)),"DatumPoint",
                      App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Coordinates of the datum point");
}

Point::~Point()
{
}

void Point::onChanged(const App::Property* prop)
{
    Datum::onChanged(prop);

    if (prop == &References) {
        std::set<QString> hint = getHint();
        if (!((hint.size() == 1) && (hint.find(QObject::tr("Point")) != hint.end())))
            return; // incomplete references

        // Extract the geometry of the references
        std::vector<App::DocumentObject*> refs = References.getValues();
        std::vector<std::string> refnames = References.getSubValues();
        Base::Vector3f* point = NULL;
        Handle_Geom_Curve c1 = NULL;
        Handle_Geom_Curve c2 = NULL;
        Handle_Geom_Surface s1 = NULL;
        Handle_Geom_Surface s2 = NULL;
        Handle_Geom_Surface s3 = NULL;

        for (int i = 0; i < refs.size(); i++) {
            if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())) {
                PartDesign::Point* p = static_cast<PartDesign::Point*>(refs[i]);
                point = new Base::Vector3d (p->_Point.getValue());
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())) {
                PartDesign::Line* l = static_cast<PartDesign::Line*>(refs[i]);
                //point = new Base::Vector3f (p->_Point.getValue());
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
                PartDesign::Plane* p = static_cast<PartDesign::Plane*>(refs[i]);
                //point = new Base::Vector3f (p->_Point.getValue());
            } else if (refs[i]->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                App::Plane* p = static_cast<App::Plane*>(refs[i]);
                //point = new Base::Vector3f (p->_Point.getValue());
            } else if (refs[i]->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                Part::Feature* feature = static_cast<Part::Feature*>(refs[i]);
                const TopoDS_Shape& sh = feature->Shape.getValue();
                if (sh.IsNull())
                    return; // "PartDesign::Point: Reference has NULL shape"
                // Get subshape
                TopoDS_Shape subshape = feature->Shape.getShape().getSubShape(refnames[i].c_str());
                if (subshape.IsNull())
                    return; // "PartDesign::Point: Reference has NULL subshape";

                if (subshape.ShapeType() == TopAbs_VERTEX) {
                    TopoDS_Vertex v = TopoDS::Vertex(subshape);
                    gp_Pnt p = BRep_Tool::Pnt(v);
                    point = new Base::Vector3f(p.X(), p.Y(), p.Z());
                } else if (subshape.ShapeType() == TopAbs_EDGE) {
                    TopoDS_Edge e = TopoDS::Edge(subshape);
                    Standard_Real first, last;
                    if (c1.IsNull())
                        c1 = BRep_Tool::Curve(e, first, last);
                    else
                        c2 = BRep_Tool::Curve(e, first, last);
                } else if (subshape.ShapeType() == TopAbs_FACE) {
                    TopoDS_Face f = TopoDS::Face(subshape);
                    if (s1.IsNull())
                        s1 = BRep_Tool::Surface(f);
                    else if (s2.IsNull())
                        s2 = BRep_Tool::Surface(f);
                    else
                        s3 = BRep_Tool::Surface(f);
                }
            } else {
                return; //"PartDesign::Point: Invalid reference type"
            }
        }

        if (point != NULL) {
            // Point from vertex or other point. Nothing to be done
        } else if (!c1.IsNull()) {
            if (!c2.IsNull()) {
                // Point from intersection of two curves
                GeomAPI_ExtremaCurveCurve intersector(c1, c2);
                if ((intersector.LowerDistance() > Precision::Confusion()) || (intersector.NbExtrema() == 0))
                    return; // No intersection
                // Note: We don't check for multiple intersection points
                gp_Pnt p, p2;
                intersector.Points(1, p, p2);
                point = new Base::Vector3f(p.X(), p.Y(), p.Z());
            } else if (!s1.IsNull()) {
                GeomAPI_IntCS intersector(c1, s1);
                if (!intersector.IsDone() || (intersector.NbPoints() == 0))
                    return;
                if (intersector.NbPoints() > 1)
                    Base::Console().Warning("More than one intersection point for datum point from curve and surface");

                gp_Pnt p = intersector.Point(1);
                point = new Base::Vector3f(p.X(), p.Y(), p.Z());
            } else
                return;
        } else if (!s1.IsNull() && !s2.IsNull() && !s3.IsNull()) {
            GeomAPI_IntSS intersectorSS(s1, s2, Precision::Confusion());
            if (!intersectorSS.IsDone() || (intersectorSS.NbLines() == 0))
                return;
            if (intersectorSS.NbLines() > 1)
                Base::Console().Warning("More than one intersection line for datum point from surfaces");
            Handle_Geom_Curve line = intersectorSS.Line(1);

            GeomAPI_IntCS intersector(line, s3);
            if (!intersector.IsDone() || (intersector.NbPoints() == 0))
                return;
            if (intersector.NbPoints() > 1)
                Base::Console().Warning("More than one intersection point for datum point from surfaces");

            gp_Pnt p = intersector.Point(1);
            point = new Base::Vector3f(p.X(), p.Y(), p.Z());
        } else {
            return;
        }

        _Point.setValue(*point);
        _Point.touch(); // This triggers ViewProvider::updateData()
        delete point;
    }
}


const std::set<QString> Point::getHint()
{
    if (hints.find(refTypes) != hints.end())
        return hints[refTypes];
    else
        return std::set<QString>();
}


PROPERTY_SOURCE(PartDesign::Line, PartDesign::Datum)

Line::Line()
{
}

Line::~Line()
{
}

void Line::onChanged(const App::Property *prop)
{
    gp_Pnt point1(0,0,0);

    gp_Pnt point2(10,10,10);

    return;
}


const std::set<QString> Line::getHint()
{
    if (hints.find(refTypes) != hints.end())
        return hints[refTypes];
    else
        return std::set<QString>();
}


PROPERTY_SOURCE(PartDesign::Plane, PartDesign::Datum)

Plane::Plane()
{
}

void Plane::onChanged(const App::Property *prop)
{
    double O = 10.0; //this->Offset.getValue();
    double A = 45.0; //this->Angle.getValue();

    if (fabs(A) > 360.0)
      return; // "Angle too large (please use -360.0 .. +360.0)"

    gp_Pnt pnt(0.0,0.0,0.0);
    gp_Dir dir(0.0,0.0,1.0);

    return;
}


const std::set<QString> Plane::getHint()
{
    if (hints.find(refTypes) != hints.end())
        return hints[refTypes];
    else
        return std::set<QString>();
}
