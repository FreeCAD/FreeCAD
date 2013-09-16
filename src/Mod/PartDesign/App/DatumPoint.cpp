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
# include <Geom_CylindricalSurface.hxx>
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
#define CYLINDER QObject::tr("DCYLINDER")
#define LINE  QObject::tr("DLINE")
#define POINT QObject::tr("DPOINT")
#define ANGLE QObject::tr("Angle")

// ================================ Initialize the hints =====================

std::map<std::multiset<QString>, std::set<QString> > Point::hints = std::map<std::multiset<QString>, std::set<QString> >();

void Point::initHints()
{
    std::set<QString> DONE;
    DONE.insert(QObject::tr("Done"));

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

// ============================================================================

PROPERTY_SOURCE(PartDesign::Point, Part::Datum)

Point::Point()
{    
    // Create a shape, which will be used by the Sketcher. Them main function is to avoid a dependency of
    // Sketcher on the PartDesign module
    BRepBuilderAPI_MakeVertex builder(gp_Pnt(0,0,0));
    if (!builder.IsDone())
        return;
    Shape.setValue(builder.Shape());

    References.touch();
}

Point::~Point()
{
}

void Point::onChanged(const App::Property* prop)
{
    if ((prop == &References) || (prop == &Offset) || (prop == &Offset2) || (prop == &Offset3)) {
        refTypes.clear();
        std::vector<App::DocumentObject*> refs = References.getValues();
        std::vector<std::string> refnames = References.getSubValues();

        for (int r = 0; r < refs.size(); r++)
            refTypes.insert(getRefType(refs[r], refnames[r]));

        std::set<QString> hint = getHint();
        if (!((hint.size() == 1) && (hint.find(QObject::tr("Done")) != hint.end())))
            return; // incomplete references

        // Extract the geometry of the references
        Base::Vector3d* point = NULL;
        Handle_Geom_Curve c1 = NULL;
        Handle_Geom_Curve c2 = NULL;
        Handle_Geom_Surface s1 = NULL;
        Handle_Geom_Surface s2 = NULL;
        Handle_Geom_Surface s3 = NULL;

        for (int i = 0; i < refs.size(); i++) {
            if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())) {
                PartDesign::Point* p = static_cast<PartDesign::Point*>(refs[i]);
                point = new Base::Vector3d (p->getPoint());
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())) {
                PartDesign::Line* l = static_cast<PartDesign::Line*>(refs[i]);
                Base::Vector3d base = l->getBasePoint();
                Base::Vector3d dir = l->getDirection();
                if (c1.IsNull())
                    c1 = new Geom_Line(gp_Pnt(base.x, base.y, base.z), gp_Dir(dir.x, dir.y, dir.z));
                else
                    c2 = new Geom_Line(gp_Pnt(base.x, base.y, base.z), gp_Dir(dir.x, dir.y, dir.z));
            } else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
                PartDesign::Plane* p = static_cast<PartDesign::Plane*>(refs[i]);
                Base::Vector3d base = p->getBasePoint();
                Base::Vector3d normal = p->getNormal();
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
                    double offset1 = Offset.getValue();
                    double offset2 = Offset2.getValue();
                    double offset3 = Offset3.getValue();
                    BRepAdaptor_Surface adapt(f);

                    if (s1.IsNull()) {
                        if (adapt.GetType() == GeomAbs_Plane) {
                            gp_Trsf mov;
                            mov.SetTranslation(offset1 * gp_Vec(adapt.Plane().Axis().Direction()));
                            TopLoc_Location loc(mov);
                            f.Move(loc);
                        }
                        s1 = BRep_Tool::Surface(f);
                    } else if (s2.IsNull()) {
                        if (adapt.GetType() == GeomAbs_Plane) {
                            gp_Trsf mov;
                            mov.SetTranslation(offset2 * gp_Vec(adapt.Plane().Axis().Direction()));
                            TopLoc_Location loc(mov);
                            f.Move(loc);
                        }
                        s2 = BRep_Tool::Surface(f);
                    } else {
                        if (adapt.GetType() == GeomAbs_Plane) {
                            gp_Trsf mov;
                            mov.SetTranslation(offset3 * gp_Vec(adapt.Plane().Axis().Direction()));
                            TopLoc_Location loc(mov);
                            f.Move(loc);
                        }
                        s3 = BRep_Tool::Surface(f);
                    }
                }
            } else {
                return; //"PartDesign::Point: Invalid reference type"
            }
        }               

        if (point != NULL) {
            // Point from vertex or other point. Nothing to be done
        } else if (circle != NULL) {
            // Point from center of circle (or arc)
            gp_Pnt centre = circle->Axis().Location();
            point = new Base::Vector3d(centre.X(), centre.Y(), centre.Z());
        } else if (!c1.IsNull()) {
            if (!c2.IsNull()) {
                // Point from intersection of two curves                                    
                GeomAPI_ExtremaCurveCurve intersector(c1, c2);
                if ((intersector.LowerDistance() > Precision::Confusion()) || (intersector.NbExtrema() == 0))
                    return; // No intersection
                // Note: We don't check for multiple intersection points
                gp_Pnt p, p2;
                intersector.Points(1, p, p2);

                // Apply offset if the curves are linear (meaning they define a plane that contains them both)
                if ((c1->DynamicType() == STANDARD_TYPE(Geom_Line)) && (c2->DynamicType() == STANDARD_TYPE(Geom_Line))) {
                    // Get translation vectors
                    Handle_Geom_Line lin1 = Handle_Geom_Line::DownCast(c1);
                    Handle_Geom_Line lin2 = Handle_Geom_Line::DownCast(c2);
                    gp_Dir normal = lin1->Lin().Direction().Crossed(lin2->Lin().Direction()); // normal of the plane
                    gp_Dir trans1 = normal.Crossed(lin1->Lin().Direction());
                    gp_Dir trans2 = normal.Crossed(lin2->Lin().Direction());
                    double offset1 = Offset.getValue();
                    double offset2 = Offset2.getValue();
                    c1->Translate(offset1 * gp_Vec(trans1));
                    c2->Translate(offset2 * gp_Vec(trans2));
                    // Intersect again
                    intersector = GeomAPI_ExtremaCurveCurve(c1, c2);
                    if ((intersector.LowerDistance() > Precision::Confusion()) || (intersector.NbExtrema() == 0))
                        return; // No intersection
                    // Note: We don't check for multiple intersection points
                    intersector.Points(1, p, p2);
                }

                point = new Base::Vector3d(p.X(), p.Y(), p.Z());
            } else if (!s1.IsNull()) {
                GeomAPI_IntCS intersector(c1, s1);
                if (!intersector.IsDone() || (intersector.NbPoints() == 0))
                    return;
                if (intersector.NbPoints() > 1)
                    Base::Console().Warning("More than one intersection point for datum point from curve and surface\n");

                gp_Pnt p = intersector.Point(1);
                point = new Base::Vector3d(p.X(), p.Y(), p.Z());
            } else
                return;
        } else if (!s1.IsNull() && !s2.IsNull() && !s3.IsNull()) {
            GeomAPI_IntSS intersectorSS(s1, s2, Precision::Confusion());
            if (!intersectorSS.IsDone() || (intersectorSS.NbLines() == 0))
                return;
            if (intersectorSS.NbLines() > 1)
                Base::Console().Warning("More than one intersection line for datum point from surfaces\n");
            Handle_Geom_Curve line = intersectorSS.Line(1);

            GeomAPI_IntCS intersector(line, s3);
            if (!intersector.IsDone() || (intersector.NbPoints() == 0))
                return;
            if (intersector.NbPoints() > 1)
                Base::Console().Warning("More than one intersection point for datum point from surfaces\n");

            gp_Pnt p = intersector.Point(1);
            point = new Base::Vector3d(p.X(), p.Y(), p.Z());
        } else {
            return;
        }

        Placement.setValue(Base::Placement(*point, Base::Rotation()));

        delete point;
    }

    Part::Datum::onChanged(prop);
}

Base::Vector3d Point::getPoint()
{
    return Placement.getValue().getPosition();
}


const std::set<QString> Point::getHint()
{
    if (hints.find(refTypes) != hints.end())
        return hints[refTypes];
    else
        return std::set<QString>();
}

const int Point::offsetsAllowed() const
{
    int numlines = 0, numplanes = 0;
    for (std::multiset<QString>::const_iterator r = refTypes.begin(); r != refTypes.end(); r++) {
        if (*r == LINE) numlines++;
        else if (*r == PLANE) numplanes++;
    }

    if (numlines == 2) return 2; // Special case: Two intersecting lines. TODO: Check for co-planarity
    return numplanes;
}


namespace PartDesign {

const QString getRefType(const App::DocumentObject* obj, const std::string& subname)
{
    Base::Type type = obj->getTypeId();

    if ((type == App::Plane::getClassTypeId()) || (type == PartDesign::Plane::getClassTypeId()))
        return PLANE;
    else if (type == PartDesign::Line::getClassTypeId())
        return LINE;
    else if (type == PartDesign::Point::getClassTypeId())
        return POINT;
    else if (type.isDerivedFrom(Part::Feature::getClassTypeId())) {        
        const Part::Feature* feature = static_cast<const Part::Feature*>(obj);
        const Part::TopoShape& topShape = feature->Shape.getShape();
        if (topShape.isNull())
            return QString::fromAscii("EMPTYSHAPE"); // Can happen on file loading
            
        if (subname.size() > 4 && subname.substr(0,4) == "Face") {                            
            TopoDS_Shape face = topShape.getSubShape(subname.c_str());
            if (face.IsNull() || (face.ShapeType() != TopAbs_FACE))
                throw Base::Exception("Part::Datum::getRefType(): No valid subshape could be extracted");
            BRepAdaptor_Surface adapt(TopoDS::Face(face));
            if (adapt.GetType() == GeomAbs_Plane)
                return PLANE;
            else if (adapt.GetType() == GeomAbs_Cylinder)
                return CYLINDER;
            else
               throw Base::Exception("Part::Datum::getRefType(): Only planar and cylindrical faces are allowed");
        } else if (subname.size() > 4 && subname.substr(0,4) == "Edge") {
            // Note: For now, only linear references are possible
            return LINE;
        } else if (subname.size() > 6 && subname.substr(0,6) == "Vertex") {
            return POINT;
        }
    }

    throw Base::Exception("Part::Datum::getRefType(): Illegal object type");
}

}
