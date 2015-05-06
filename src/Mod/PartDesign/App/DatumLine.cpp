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
# include <gp_Ax3.hxx>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <gp_Circ.hxx>
# include <gp_Cylinder.hxx>
# include <Geom_Line.hxx>
# include <Geom_Plane.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom2d_Line.hxx>
# include <Handle_Geom_Curve.hxx>
# include <Handle_Geom_Surface.hxx>
# include <Handle_Geom_Plane.hxx>
# include <Handle_Geom2d_Line.hxx>
# include <Handle_Standard_Type.hxx>
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
#include <App/Part.h>
#include <App/Line.h>
#include "DatumPoint.h"
#include "DatumLine.h"
#include "DatumPlane.h"
#include <Base/Tools.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include "Mod/Part/App/PrimitiveFeature.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;

// Note: We don't distinguish between e.g. datum lines and edges here
#define PLANE QObject::tr("DPLANE")
#define LINE  QObject::tr("DLINE")
#define CIRCLE QObject::tr("DCIRCLE")
#define POINT QObject::tr("DPOINT")
#define ANGLE QObject::tr("Angle")

std::map<std::multiset<QString>, std::set<QString> > Line::hints = std::map<std::multiset<QString>, std::set<QString> >();

void Line::initHints()
{
    std::set<QString> DONE;
    DONE.insert(QObject::tr("Done"));

    std::multiset<QString> key;
    std::set<QString> value;
    key.insert(LINE);
    hints[key] = DONE; // LINE -> DONE. Line from another line or edge

    key.clear(); value.clear();
    key.insert(CIRCLE);
    hints[key] = DONE; // CIRCLE -> DONE. Line from center of circle or arc or axis of cylinder or cylinder segment

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
    value.insert(LINE);
    hints[key] = value; // PLANE -> LINE or PLANE

    key.clear(); value.clear();
    key.insert(PLANE); key.insert(PLANE);
    hints[key] = DONE; // {PLANE, PLANE} -> DONE. Line from two planes or faces

    key.clear(); value.clear();
    key.insert(PLANE); key.insert(LINE);
    value.insert(LINE);
    hints[key] = value; // {PLANE, LINE} -> LINE

    key.clear(); value.clear();
    key.insert(PLANE); key.insert(LINE); key.insert(LINE);
    hints[key] = DONE; // {PLANE, LINE, LINE} -> DONE. Line from plane with distance (default zero) to two lines

    key.clear(); value.clear();
    value.insert(POINT); value.insert(LINE); value.insert(PLANE);
    hints[key] = value;
}

PROPERTY_SOURCE(PartDesign::Line, Part::Datum)

Line::Line()
{
    // Create a shape, which will be used by the Sketcher. Them main function is to avoid a dependency of
    // Sketcher on the PartDesign module
    BRepBuilderAPI_MakeEdge builder(gp_Lin(gp_Pnt(0,0,0), gp_Dir(0,0,1)));
    if (!builder.IsDone())
        return;
    Shape.setValue(builder.Shape());

    References.touch();
}

Line::~Line()
{
}

void Line::onChanged(const App::Property *prop)
{
    if ((prop == &References) || (prop == &Offset) || (prop == &Offset2)) {
        refTypes.clear();
        std::vector<App::DocumentObject*> refs = References.getValues();
        std::vector<std::string> refnames = References.getSubValues();
        if (refs.size() != refnames.size())
            return;

        for (int r = 0; r < refs.size(); r++)
            refTypes.insert(getRefType(refs[r], refnames[r]));

        std::set<QString> hint = getHint();
        if (!((hint.size() == 1) && (hint.find(QObject::tr("Done")) != hint.end())))
            return; // incomplete references

        // Extract the geometry of the references
        Base::Vector3d* base = NULL;
        Base::Vector3d* direction = NULL;
        Base::Vector3d* p1 = NULL;
        Base::Vector3d* p2 = NULL;
        gp_Lin* line = NULL;
        gp_Circ* circle = NULL;
        Handle_Geom_Surface s1 = NULL;
        Handle_Geom_Surface s2 = NULL;
        Handle_Geom_Surface s3 = NULL;

        for (int i = 0; i < refs.size(); i++) {
            if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())) {
                PartDesign::Point* p = static_cast<PartDesign::Point*>(refs[i]);
                if (p1 == NULL)
                    p1 = new Base::Vector3d (p->getPoint());
                else
                    p2 = new Base::Vector3d (p->getPoint());
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())) {
                PartDesign::Line* l = static_cast<PartDesign::Line*>(refs[i]);
                if (s1.IsNull()) {
                    base = new Base::Vector3d (l->getBasePoint());
                    direction = new Base::Vector3d (l->getDirection());
                } else {
                    // Create plane through line normal to s1
                    Handle_Geom_Plane pl = Handle_Geom_Plane::DownCast(s1);
                    if (pl.IsNull())
                        return; // Non-planar first surface
                    gp_Dir ldir = gp_Dir(l->getDirection().x, l->getDirection().y, l->getDirection().z);
                    gp_Dir normal = ldir.Crossed(pl->Axis().Direction());
                    double offset1 = Offset.getValue();
                    double offset2 = Offset2.getValue();
                    gp_Pnt base = gp_Pnt(l->getBasePoint().x, l->getBasePoint().y, l->getBasePoint().z);
                    if (s2.IsNull()) {
                        base.Translate(offset1 * normal);
                        s2 = new Geom_Plane(base, normal);
                    } else {
                        base.Translate(offset2 * normal);
                        s3 = new Geom_Plane(base, normal);
                    }
                }
            } else if (refs[i]->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
                App::Line* l = static_cast<App::Line*>(refs[i]);
                gp_Dir ldir;
                if (strcmp(l->getNameInDocument(), App::Part::BaselineTypes[0]) == 0)
                    ldir = gp_Dir(1,0,0);
                else if (strcmp(l->getNameInDocument(), App::Part::BaselineTypes[1]) == 0)
                    ldir = gp_Dir(0,1,0);
                else if (strcmp(l->getNameInDocument(), App::Part::BaselineTypes[2]) == 0)
                    ldir = gp_Dir(0,0,1);
                
                if (s1.IsNull()) {
                    base = new Base::Vector3d (0,0,0);
                    direction = new Base::Vector3d (ldir.X(), ldir.Y(), ldir.Z());
                } else {
                    // Create plane through line normal to s1
                    Handle_Geom_Plane pl = Handle_Geom_Plane::DownCast(s1);
                    if (pl.IsNull())
                        return; // Non-planar first surface
                    gp_Dir normal = ldir.Crossed(pl->Axis().Direction());
                    double offset1 = Offset.getValue();
                    double offset2 = Offset2.getValue();
                    gp_Pnt base = gp_Pnt(0,0,0);
                    if (s2.IsNull()) {
                        base.Translate(offset1 * normal);
                        s2 = new Geom_Plane(base, normal);
                    } else {
                        base.Translate(offset2 * normal);
                        s3 = new Geom_Plane(base, normal);
                    }
                }
                
            }else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
                PartDesign::Plane* p = static_cast<PartDesign::Plane*>(refs[i]);
                Base::Vector3d base = p->getBasePoint();
                Base::Vector3d normal = p->getNormal();
                double offset1 = Offset.getValue();
                double offset2 = Offset2.getValue();

                if (s1.IsNull()) {
                    base += normal * offset1;
                    s1 = new Geom_Plane(gp_Pnt(base.x, base.y, base.z), gp_Dir(normal.x, normal.y, normal.z));
                } else {
                    base += normal * offset2;
                    s2 = new Geom_Plane(gp_Pnt(base.x, base.y, base.z), gp_Dir(normal.x, normal.y, normal.z));
                }
            } else if (refs[i]->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                App::Plane* p = static_cast<App::Plane*>(refs[i]);
                // Note: We only handle the three base planes here
                gp_Pnt base(0,0,0);
                gp_Dir normal;
                if (strcmp(p->getNameInDocument(), App::Part::BaseplaneTypes[0]) == 0)
                    normal = gp_Dir(0,0,1);
                else if (strcmp(p->getNameInDocument(), App::Part::BaseplaneTypes[2]) == 0)
                    normal = gp_Dir(1,0,0);
                else if (strcmp(p->getNameInDocument(), App::Part::BaseplaneTypes[1]) == 0)
                    normal = gp_Dir(0,1,0);

                double offset1 = Offset.getValue();
                double offset2 = Offset2.getValue();

                if (s1.IsNull()) {
                    base = gp_Pnt(normal.X() * offset1, normal.Y() * offset1, normal.Z() * offset1);
                    s1 = new Geom_Plane(base, normal);
                } else {
                    base = gp_Pnt(normal.X() * offset2, normal.Y() * offset2, normal.Z() * offset2);
                    s2 = new Geom_Plane(base, normal);
                }
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
                    if (adapt.GetType() == GeomAbs_Circle) {
                        circle = new gp_Circ(adapt.Circle());
                    } else if (adapt.GetType() == GeomAbs_Line) {
                        if (s1.IsNull()) {
                            line = new gp_Lin(adapt.Line());
                        } else {
                            // Create plane through line normal to s1
                            Handle_Geom_Plane pl = Handle_Geom_Plane::DownCast(s1);
                            if (pl.IsNull())
                                return; // Non-planar first surface
                            gp_Dir normal = adapt.Line().Direction().Crossed(pl->Axis().Direction());
                            double offset1 = Offset.getValue();
                            double offset2 = Offset2.getValue();
                            gp_Pnt base = adapt.Line().Location();
                            if (s2.IsNull()) {
                                base.Translate(offset1 * normal);
                                s2 = new Geom_Plane(base, normal);
                            } else {
                                base.Translate(offset2 * normal);
                                s3 = new Geom_Plane(base, normal);
                            }
                        }
                    } else {
                        return; // Non-linear edge
                    }
                } else if (subshape.ShapeType() == TopAbs_FACE) {
                    TopoDS_Face f = TopoDS::Face(subshape);
                    double offset1 = Offset.getValue();
                    double offset2 = Offset2.getValue();
                    BRepAdaptor_Surface adapt(f);

                    if (s1.IsNull()) {
                        if (adapt.GetType() == GeomAbs_Cylinder) {
                            circle = new gp_Circ(gp_Ax2(adapt.Cylinder().Location(), adapt.Cylinder().Axis().Direction()),
                                                 adapt.Cylinder().Radius());
                        } else if (adapt.GetType() == GeomAbs_Plane) {
                            gp_Trsf mov;
                            mov.SetTranslation(offset1 * gp_Vec(adapt.Plane().Axis().Direction()));
                            TopLoc_Location loc(mov);
                            f.Move(loc);
                        }
                        s1 = BRep_Tool::Surface(f);
                    } else {
                        if (adapt.GetType() == GeomAbs_Plane) {
                            gp_Trsf mov;
                            mov.SetTranslation(offset2 * gp_Vec(adapt.Plane().Axis().Direction()));
                            TopLoc_Location loc(mov);
                            f.Move(loc);
                        }
                        s2 = BRep_Tool::Surface(f);
                    }
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
        } else if (circle != NULL) {
            // Line from center of circle or cylinder
            gp_Pnt centre = circle->Axis().Location();
            base = new Base::Vector3d(centre.X(), centre.Y(), centre.Z());
            gp_Dir dir = circle->Axis().Direction();
            direction = new Base::Vector3d(dir.X(), dir.Y(), dir.Z());
        }  else if (!s1.IsNull() && !s2.IsNull()) {
            if (!s3.IsNull())
                s1 = s3; // Line from a plane and two lines/edges
            // Line from two surfaces
            GeomAPI_IntSS intersectorSS(s1, s2, Precision::Confusion());
            if (!intersectorSS.IsDone() || (intersectorSS.NbLines() == 0))
                return;
            if (intersectorSS.NbLines() > 1)
                Base::Console().Warning("More than one intersection curve for datum line from surfaces\n");
            Handle_Geom_Line l = Handle_Geom_Line::DownCast(intersectorSS.Line(1));
            if (l.IsNull())
                return; // non-linear intersection curve
            gp_Lin lin = l->Lin();
            base = new Base::Vector3d(lin.Location().X(), lin.Location().Y(), lin.Location().Z());
            direction = new Base::Vector3d(lin.Direction().X(), lin.Direction().Y(), lin.Direction().Z());
        } else {
            return;
        }

        Placement.setValue(Base::Placement(*base, Base::Rotation(Base::Vector3d(0,0,1), *direction)));

        delete base;
        delete direction;
        if (p1 != NULL) delete p1;
        if (p2 != NULL) delete p2;
        if (line != NULL) delete line;
        if (circle != NULL) delete circle;
    }

    Part::Datum::onChanged(prop);
}

const std::set<QString> Line::getHint() const
{
    if (hints.find(refTypes) != hints.end())
        return hints[refTypes];
    else
        return std::set<QString>();
}

const int Line::offsetsAllowed() const
{
    int planes = 0;
    int lines = 0;
    for (std::multiset<QString>::const_iterator r = refTypes.begin(); r != refTypes.end(); r++) {
        if (*r == PLANE) planes++;
        if (*r == LINE) lines++;
    }
    if (lines == 0) return planes;
    if ((planes == 1) && (lines == 2)) return 2;
    return 0;
}

Base::Vector3d Line::getBasePoint() const
{
    return Placement.getValue().getPosition();
}

Base::Vector3d Line::getDirection() const
{
    Base::Rotation rot = Placement.getValue().getRotation();
    Base::Vector3d dir;
    rot.multVec(Base::Vector3d(0,0,1), dir);
    return dir;
}
