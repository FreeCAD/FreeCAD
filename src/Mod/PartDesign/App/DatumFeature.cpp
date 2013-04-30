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
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <Geom_Line.hxx>
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
#include <Base/Console.h>
#include <Base/Exception.h>
#include "Mod/Part/App/PrimitiveFeature.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;

PROPERTY_SOURCE_ABSTRACT(PartDesign::Datum, App::GeoFeature)

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

    App::GeoFeature::onChanged(prop);
}

void Datum::onDocumentRestored()
{
    // This seems to be the only way to make the ViewProvider display the datum feature
    References.touch();
    App::GeoFeature::onDocumentRestored();
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
    ADD_PROPERTY_TYPE(_Point,(Base::Vector3d(0,0,0)),"DatumPoint",
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
        Base::Vector3d* point = NULL;
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
                Base::Vector3d base = l->_Base.getValue();
                Base::Vector3d dir = l->_Direction.getValue();
                if (c1.IsNull())
                    c1 = new Geom_Line(gp_Pnt(base.x, base.y, base.z), gp_Dir(dir.x, dir.y, dir.z));
                else
                    c2 = new Geom_Line(gp_Pnt(base.x, base.y, base.z), gp_Dir(dir.x, dir.y, dir.z));
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
                PartDesign::Plane* p = static_cast<PartDesign::Plane*>(refs[i]);
                Base::Vector3d base = p->_Base.getValue();
                Base::Vector3d normal = p->_Normal.getValue();
                if (s1.IsNull())
                    s1 = new Geom_Plane(gp_Pnt(base.x, base.y, base.z), gp_Dir(normal.x, normal.y, normal.z));
                else if (s2.IsNull())
                    s2 = new Geom_Plane(gp_Pnt(base.x, base.y, base.z), gp_Dir(normal.x, normal.y, normal.z));
                else
                    s3 = new Geom_Plane(gp_Pnt(base.x, base.y, base.z), gp_Dir(normal.x, normal.y, normal.z));
            } else if (refs[i]->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                App::Plane* p = static_cast<App::Plane*>(refs[i]);
                // Note: We only handle the three base planes here
                gp_Pnt base(0,0,0);
                gp_Dir normal;
                if (strcmp(p->getNameInDocument(), "BaseplaneXY") == 0)
                    normal = gp_Dir(0,0,1);
                else if (strcmp(p->getNameInDocument(), "BaseplaneYZ") == 0)
                    normal = gp_Dir(1,0,0);
                else if (strcmp(p->getNameInDocument(), "BaseplaneXZ") == 0)
                    normal = gp_Dir(0,1,0);

                if (s1.IsNull())
                    s1 = new Geom_Plane(base, normal);
                else if (s2.IsNull())
                    s2 = new Geom_Plane(base, normal);
                else
                    s3 = new Geom_Plane(base, normal);
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
                    point = new Base::Vector3d(p.X(), p.Y(), p.Z());
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
                point = new Base::Vector3d(p.X(), p.Y(), p.Z());
            } else if (!s1.IsNull()) {
                GeomAPI_IntCS intersector(c1, s1);
                if (!intersector.IsDone() || (intersector.NbPoints() == 0))
                    return;
                if (intersector.NbPoints() > 1)
                    Base::Console().Warning("More than one intersection point for datum point from curve and surface");

                gp_Pnt p = intersector.Point(1);
                point = new Base::Vector3d(p.X(), p.Y(), p.Z());
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
            point = new Base::Vector3d(p.X(), p.Y(), p.Z());
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
    ADD_PROPERTY_TYPE(_Base,(Base::Vector3d(0,0,0)),"DatumLine",
                      App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Coordinates of the line base point");
    ADD_PROPERTY_TYPE(_Direction,(Base::Vector3d(1,1,1)),"DatumLine",
                      App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Coordinates of the line direction");
}

Line::~Line()
{
}

void Line::onChanged(const App::Property *prop)
{
    Datum::onChanged(prop);

    if (prop == &References) {
        std::set<QString> hint = getHint();
        if (!((hint.size() == 1) && (hint.find(QObject::tr("Line")) != hint.end())))
            return; // incomplete references

        // Extract the geometry of the references
        std::vector<App::DocumentObject*> refs = References.getValues();
        std::vector<std::string> refnames = References.getSubValues();
        Base::Vector3d* base = NULL;
        Base::Vector3d* direction = NULL;
        Base::Vector3d* p1 = NULL;
        Base::Vector3d* p2 = NULL;
        gp_Lin* line = NULL;
        Handle_Geom_Surface s1 = NULL;
        Handle_Geom_Surface s2 = NULL;

        for (int i = 0; i < refs.size(); i++) {
            if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())) {
                PartDesign::Point* p = static_cast<PartDesign::Point*>(refs[i]);
                if (p1 == NULL)
                    p1 = new Base::Vector3d (p->_Point.getValue());
                else
                    p2 = new Base::Vector3d (p->_Point.getValue());
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())) {
                PartDesign::Line* l = static_cast<PartDesign::Line*>(refs[i]);
                base = new Base::Vector3d (l->_Base.getValue());
                direction = new Base::Vector3d (l->_Direction.getValue());
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
                PartDesign::Plane* p = static_cast<PartDesign::Plane*>(refs[i]);
                Base::Vector3d base = p->_Base.getValue();
                Base::Vector3d normal = p->_Normal.getValue();
                if (s1.IsNull())
                    s1 = new Geom_Plane(gp_Pnt(base.x, base.y, base.z), gp_Dir(normal.x, normal.y, normal.z));
                else
                    s2 = new Geom_Plane(gp_Pnt(base.x, base.y, base.z), gp_Dir(normal.x, normal.y, normal.z));
            } else if (refs[i]->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                App::Plane* p = static_cast<App::Plane*>(refs[i]);
                // Note: We only handle the three base planes here
                gp_Pnt base(0,0,0);
                gp_Dir normal;
                if (strcmp(p->getNameInDocument(), "BaseplaneXY") == 0)
                    normal = gp_Dir(0,0,1);
                else if (strcmp(p->getNameInDocument(), "BaseplaneYZ") == 0)
                    normal = gp_Dir(1,0,0);
                else if (strcmp(p->getNameInDocument(), "BaseplaneXZ") == 0)
                    normal = gp_Dir(0,1,0);

                if (s1.IsNull())
                    s1 = new Geom_Plane(base, normal);
                else
                    s2 = new Geom_Plane(base, normal);
            } else if (refs[i]->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                Part::Feature* feature = static_cast<Part::Feature*>(refs[i]);
                const TopoDS_Shape& sh = feature->Shape.getValue();
                if (sh.IsNull())
                    return; // "PartDesign::Line: Reference has NULL shape"
                // Get subshape
                TopoDS_Shape subshape = feature->Shape.getShape().getSubShape(refnames[i].c_str());
                if (subshape.IsNull())
                    return; // "PartDesign::Line: Reference has NULL subshape";

                if (subshape.ShapeType() == TopAbs_VERTEX) {
                    TopoDS_Vertex v = TopoDS::Vertex(subshape);
                    gp_Pnt p = BRep_Tool::Pnt(v);
                    if (p1 == NULL)
                        p1 = new Base::Vector3d(p.X(), p.Y(), p.Z());
                    else
                        p2 = new Base::Vector3d(p.X(), p.Y(), p.Z());
                } else if (subshape.ShapeType() == TopAbs_EDGE) {
                    TopoDS_Edge e = TopoDS::Edge(subshape);
                    BRepAdaptor_Curve adapt(e);
                    if (adapt.GetType() != GeomAbs_Line)
                        return; // Non-linear edge
                    line = new gp_Lin(adapt.Line());
                } else if (subshape.ShapeType() == TopAbs_FACE) {
                    TopoDS_Face f = TopoDS::Face(subshape);
                    if (s1.IsNull())
                        s1 = BRep_Tool::Surface(f);
                    else
                        s2 = BRep_Tool::Surface(f);
                }
            } else {
                return; //"PartDesign::Point: Invalid reference type"
            }
        }

        if ((base != NULL) && (direction != NULL)) {
            // Line from other datum line. Nothing to be done
        } else if ((p1 != NULL) && (p2 != NULL)) {
            // Line from two points
            base = new Base::Vector3d(*p1);
            direction = new Base::Vector3d(*p2 - *p1);
        } else if (line != NULL) {
            // Line from gp_lin
            base = new Base::Vector3d(line->Location().X(), line->Location().Y(), line->Location().Z());
            direction = new Base::Vector3d(line->Direction().X(), line->Direction().Y(), line->Direction().Z());
        } else if (!s1.IsNull() && !s2.IsNull()) {
            GeomAPI_IntSS intersectorSS(s1, s2, Precision::Confusion());
            if (!intersectorSS.IsDone() || (intersectorSS.NbLines() == 0))
                return;
            if (intersectorSS.NbLines() > 1)
                Base::Console().Warning("More than one intersection line for datum point from surfaces");
            Handle_Geom_Line l = Handle_Geom_Line::DownCast(intersectorSS.Line(1));
            if (l.IsNull())
                return; // non-linear intersection curve
            gp_Lin lin = l->Lin();
            base = new Base::Vector3d(lin.Location().X(), lin.Location().Y(), lin.Location().Z());
            direction = new Base::Vector3d(lin.Direction().X(), lin.Direction().Y(), lin.Direction().Z());
        } else {
            return;
        }

        _Base.setValue(*base);
        _Direction.setValue(*direction);
        _Base.touch(); // This triggers ViewProvider::updateData()
        delete base;
        delete direction;
        if (p1 != NULL) delete p1;
        if (p2 != NULL) delete p2;
        if (line != NULL) delete line;
    }
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
ADD_PROPERTY_TYPE(_Base,(Base::Vector3d(0,0,0)),"DatumPlane",
                  App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                  "Coordinates of the plane base point");
ADD_PROPERTY_TYPE(_Normal,(Base::Vector3d(1,1,1)),"DatumPlane",
                  App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                  "Coordinates of the plane normal");
}

void Plane::onChanged(const App::Property *prop)
{
    Datum::onChanged(prop);

    if (prop == &References) {
        std::set<QString> hint = getHint();
        if (!((hint.size() == 1) && (hint.find(QObject::tr("Plane")) != hint.end())))
            return; // incomplete references

        // Extract the geometry of the references
        std::vector<App::DocumentObject*> refs = References.getValues();
        std::vector<std::string> refnames = References.getSubValues();
        Base::Vector3d* p1 = NULL;
        Base::Vector3d* p2 = NULL;
        Base::Vector3d* p3 = NULL;
        Base::Vector3d* normal = NULL;
        gp_Lin* line = NULL;

        for (int i = 0; i < refs.size(); i++) {
            if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())) {
                PartDesign::Point* p = static_cast<PartDesign::Point*>(refs[i]);
                if (p1 == NULL)
                    p1 = new Base::Vector3d (p->_Point.getValue());
                else if (p2 == NULL)
                    p2 = new Base::Vector3d (p->_Point.getValue());
                else
                    p3 = new Base::Vector3d (p->_Point.getValue());
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())) {
                PartDesign::Line* l = static_cast<PartDesign::Line*>(refs[i]);
                Base::Vector3d base = l->_Base.getValue();
                Base::Vector3d dir = l->_Direction.getValue();
                line = new gp_Lin(gp_Pnt(base.x, base.y, base.z), gp_Dir(dir.x, dir.y, dir.z));
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
                PartDesign::Plane* p = static_cast<PartDesign::Plane*>(refs[i]);
                p1 = new Base::Vector3d(p->_Base.getValue());
                normal = new Base::Vector3d(p->_Normal.getValue());
            } else if (refs[i]->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                App::Plane* p = static_cast<App::Plane*>(refs[i]);
                // Note: We only handle the three base planes here
                p1 = new Base::Vector3d(0,0,0);
                normal = new Base::Vector3d;
                if (strcmp(p->getNameInDocument(), "BaseplaneXY") == 0)
                    *normal = Base::Vector3d(0,0,1);
                else if (strcmp(p->getNameInDocument(), "BaseplaneYZ") == 0)
                    *normal = Base::Vector3d(1,0,0);
                else if (strcmp(p->getNameInDocument(), "BaseplaneXZ") == 0)
                    *normal = Base::Vector3d(0,1,0);
            } else if (refs[i]->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                Part::Feature* feature = static_cast<Part::Feature*>(refs[i]);
                const TopoDS_Shape& sh = feature->Shape.getValue();
                if (sh.IsNull())
                    return; // "PartDesign::Plane: Reference has NULL shape"
                // Get subshape
                TopoDS_Shape subshape = feature->Shape.getShape().getSubShape(refnames[i].c_str());
                if (subshape.IsNull())
                    return; // "PartDesign::Plane: Reference has NULL subshape";

                if (subshape.ShapeType() == TopAbs_VERTEX) {
                    TopoDS_Vertex v = TopoDS::Vertex(subshape);
                    gp_Pnt p = BRep_Tool::Pnt(v);
                    if (p1 == NULL)
                        p1 = new Base::Vector3d(p.X(), p.Y(), p.Z());
                    else if (p2 == NULL)
                        p2 = new Base::Vector3d(p.X(), p.Y(), p.Z());
                    else
                        p3 = new Base::Vector3d(p.X(), p.Y(), p.Z());
                } else if (subshape.ShapeType() == TopAbs_EDGE) {
                    TopoDS_Edge e = TopoDS::Edge(subshape);
                    BRepAdaptor_Curve adapt(e);
                    if (adapt.GetType() != GeomAbs_Line)
                        return; // Non-linear edge
                    line = new gp_Lin(adapt.Line());
                } else if (subshape.ShapeType() == TopAbs_FACE) {
                    TopoDS_Face f = TopoDS::Face(subshape);
                    BRepAdaptor_Surface adapt(f);
                    if (adapt.GetType() != GeomAbs_Plane)
                        return; // Non-planar face
                    gp_Pnt b = adapt.Plane().Location();
                    gp_Dir d = adapt.Plane().Axis().Direction();
                    p1 = new Base::Vector3d(b.X(), b.Y(), b.Z());
                    normal = new Base::Vector3d(d.X(), d.Y(), d.Z());
                }
            } else {
                return; //"PartDesign::Plane: Invalid reference type"
            }
        }

        if ((p1 != NULL) && (normal != NULL)) {
            // plane from other plane. Nothing to be done
        } else if ((p1 != NULL) && (p2 != NULL) && (p3 != NULL)) {
            // Plane from three points
            Base::Vector3d vec1 = *p2 - *p1;
            Base::Vector3d vec2 = *p3 - *p1;
            normal = new Base::Vector3d(vec1 % vec2);
        } else if ((line != NULL) && (p1 != NULL)) {
            // Plane from point and line
            p2 = new Base::Vector3d(line->Location().X(), line->Location().Y(), line->Location().Z());
            gp_Pnt p(line->Location().X() + line->Direction().X(), line->Location().Y() + line->Direction().Y(), line->Location().Z() + line->Direction().Z());
            p3 = new Base::Vector3d(p.X(), p.Y(), p.Z());
            Base::Vector3d vec1 = *p2 - *p1;
            Base::Vector3d vec2 = *p3 - *p1;
            normal = new Base::Vector3d(vec1 % vec2);
        } else {
            return;
        }

        _Base.setValue(*p1);
        _Normal.setValue(*normal);
        _Base.touch(); // This triggers ViewProvider::updateData()
        delete p1;
        delete normal;
        if (p2 != NULL) delete p2;
        if (p3 != NULL) delete p3;
        if (line != NULL) delete line;
    }
}


const std::set<QString> Plane::getHint()
{
    if (hints.find(refTypes) != hints.end())
        return hints[refTypes];
    else
        return std::set<QString>();
}
