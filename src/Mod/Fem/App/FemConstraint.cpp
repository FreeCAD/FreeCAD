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
# include <TopoDS.hxx>
# include <BRepGProp_Face.hxx>
# include <gp_Vec.hxx>
# include <gp_Pnt.hxx>
# include <gp_Pln.hxx>
# include <gp_Cylinder.hxx>
# include <gp_Ax3.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <GCPnts_AbscissaPoint.hxx>
# include <Adaptor3d_IsoCurve.hxx>
# include <Adaptor3d_HSurface.hxx>
# include <BRepAdaptor_HSurface.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <GProp_GProps.hxx>
# include <BRepGProp.hxx>
# include <TopoDS_Vertex.hxx>
# include <BRepClass_FaceClassifier.hxx>
# include <BRep_Tool.hxx>
# include <BRepGProp_Face.hxx>
# include <ShapeAnalysis.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <GeomAPI_IntCS.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Line.hxx>
# include <Precision.hxx>
#endif

#include "FemConstraint.h"

#include <Mod/Part/App/PartFeature.h>
#include <Base/Console.h>

using namespace Fem;

// maybe in the c++ standard later, older compiler don't have round()
double round(double r) {
    return (r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5);
}

PROPERTY_SOURCE(Fem::Constraint, App::DocumentObject);

Constraint::Constraint()
{
    ADD_PROPERTY_TYPE(References,(0,0),"Constraint",(App::PropertyType)(App::Prop_None),"Elements where the constraint is applied");
    ADD_PROPERTY_TYPE(NormalDirection,(Base::Vector3d(0,0,1)),"Constraint",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),"Normal direction pointing outside of solid");
}

Constraint::~Constraint()
{
}

App::DocumentObjectExecReturn *Constraint::execute(void)
{
    References.touch();
    return StdReturn;
}

void Constraint::onChanged(const App::Property* prop)
{
    //Base::Console().Error("Constraint::onChanged() %s\n", prop->getName());
    if (prop == &References) {
        // If References are changed, recalculate the normal direction. If no useful reference is found,
        // use z axis or previous value. If several faces are selected, only the first one is used
        std::vector<App::DocumentObject*> Objects = References.getValues();
        std::vector<std::string> SubElements = References.getSubValues();

        // Extract geometry from References
        TopoDS_Shape sh;

        for (std::size_t i = 0; i < Objects.size(); i++) {
            App::DocumentObject* obj = Objects[i];
            Part::Feature* feat = static_cast<Part::Feature*>(obj);
            const Part::TopoShape& toposhape = feat->Shape.getShape();
            if (!toposhape._Shape.IsNull()) {
                sh = toposhape.getSubShape(SubElements[i].c_str());

                if (sh.ShapeType() == TopAbs_FACE) {
                    // Get face normal in center point
                    TopoDS_Face face = TopoDS::Face(sh);
                    BRepGProp_Face props(face);
                    gp_Vec normal;
                    gp_Pnt center;
                    double u1,u2,v1,v2;
                    props.Bounds(u1,u2,v1,v2);
                    props.Normal((u1+u2)/2.0,(v1+v2)/2.0,center,normal);
                    normal.Normalize();
                    NormalDirection.setValue(normal.X(), normal.Y(), normal.Z());
                    // One face is enough...
                    App::DocumentObject::onChanged(prop);
                    return;
                }
            }
        }
    }

    App::DocumentObject::onChanged(prop);
}

void Constraint::onDocumentRestored()
{
    // This seems to be the only way to make the ViewProvider display the constraint
    References.touch();
    App::DocumentObject::onDocumentRestored();
}

const bool Constraint::getPoints(std::vector<Base::Vector3d> &points, std::vector<Base::Vector3d> &normals) const
{
    std::vector<App::DocumentObject*> Objects = References.getValues();
    std::vector<std::string> SubElements = References.getSubValues();

    // Extract geometry from References
    TopoDS_Shape sh;

    for (std::size_t i = 0; i < Objects.size(); i++) {
        App::DocumentObject* obj = Objects[i];
        Part::Feature* feat = static_cast<Part::Feature*>(obj);
        const Part::TopoShape& toposhape = feat->Shape.getShape();
        if (toposhape.isNull())
            return false;
        sh = toposhape.getSubShape(SubElements[i].c_str());

        if (sh.ShapeType() == TopAbs_VERTEX) {
            const TopoDS_Vertex& vertex = TopoDS::Vertex(sh);
            gp_Pnt p = BRep_Tool::Pnt(vertex);
            points.push_back(Base::Vector3d(p.X(), p.Y(), p.Z()));
            normals.push_back(NormalDirection.getValue());
        } else if (sh.ShapeType() == TopAbs_EDGE) {
            BRepAdaptor_Curve curve(TopoDS::Edge(sh));
            double fp = curve.FirstParameter();
            double lp = curve.LastParameter();
            GProp_GProps props;
            BRepGProp::LinearProperties(TopoDS::Edge(sh), props);
            double l = props.Mass();
            // Create points with 10 units distance, but at least one at the beginning and end of the edge
            int steps;
            if (l >= 20)
                steps = (int)round(l / 10);
            else
                steps = 1;
            double step = (lp - fp) / steps;
            for (int i = 0; i < steps + 1; i++) {
                gp_Pnt p = curve.Value(i * step);
                points.push_back(Base::Vector3d(p.X(), p.Y(), p.Z()));
                normals.push_back(NormalDirection.getValue());
            }
        } else if (sh.ShapeType() == TopAbs_FACE) {
            TopoDS_Face face = TopoDS::Face(sh);
            // Surface boundaries
            BRepAdaptor_Surface surface(face);
            double ufp = surface.FirstUParameter();
            double ulp = surface.LastUParameter();
            double vfp = surface.FirstVParameter();
            double vlp = surface.LastVParameter();
            // Surface normals
            BRepGProp_Face props(face);
            gp_Vec normal;
            gp_Pnt center;
            // Get an estimate for the number of arrows by finding the average length of curves
            Handle(Adaptor3d_HSurface) hsurf;
            hsurf = new BRepAdaptor_HSurface(surface);
            Adaptor3d_IsoCurve isoc(hsurf, GeomAbs_IsoU, vfp);
            double l = GCPnts_AbscissaPoint::Length(isoc, Precision::Confusion());
            isoc.Load(GeomAbs_IsoU, vlp);
            double lv = (l + GCPnts_AbscissaPoint::Length(isoc, Precision::Confusion()))/2.0;
            isoc.Load(GeomAbs_IsoV, ufp);
            l = GCPnts_AbscissaPoint::Length(isoc, Precision::Confusion());
            isoc.Load(GeomAbs_IsoV, ulp);
            double lu = (l + GCPnts_AbscissaPoint::Length(isoc, Precision::Confusion()))/2.0;
            int stepsv;
            if (lv >= 20.0)
                stepsv = (int)round(lv / 10);
            else
                stepsv = 2; // Minimum of three arrows to ensure (as much as possible) that at least one is displayed
            int stepsu;
            if (lu >= 20.0)
                stepsu = (int)round(lu / 10);
            else
                stepsu = 2;
            double stepv = (vlp - vfp) / stepsv;
            double stepu = (ulp - ufp) / stepsu;
            // Create points and normals
            for (int i = 0; i < stepsv + 1; i++) {
                for (int j = 0; j < stepsu + 1; j++) {
                    double v = vfp + i * stepv;
                    double u = ufp + j * stepu;
                    gp_Pnt p = surface.Value(u, v);
                    BRepClass_FaceClassifier classifier(face, p, Precision::Confusion());
                    if (classifier.State() != TopAbs_OUT) {
                        points.push_back(Base::Vector3d(p.X(), p.Y(), p.Z()));
                        props.Normal(u, v,center,normal);
                        normal.Normalize();
                        normals.push_back(Base::Vector3d(normal.X(), normal.Y(), normal.Z()));
                    }
                }
            }
        }
    }

    return true;
}

const bool Constraint::getCylinder(double &radius, double &height, Base::Vector3d& base, Base::Vector3d& axis) const
{
    std::vector<App::DocumentObject*> Objects = References.getValues();
    std::vector<std::string> SubElements = References.getSubValues();
    if (Objects.empty())
        return false;
    App::DocumentObject* obj = Objects[0];
    Part::Feature* feat = static_cast<Part::Feature*>(obj);
    Part::TopoShape toposhape = feat->Shape.getShape();
    if (toposhape.isNull())
        return false;
    TopoDS_Shape sh = toposhape.getSubShape(SubElements[0].c_str());

    TopoDS_Face face = TopoDS::Face(sh);
    BRepAdaptor_Surface surface(face);
    gp_Cylinder cyl = surface.Cylinder();
    gp_Pnt start = surface.Value(surface.FirstUParameter(), surface.FirstVParameter());
    gp_Pnt end   = surface.Value(surface.FirstUParameter(), surface.LastVParameter());
    height = start.Distance(end);
    radius = cyl.Radius();

    gp_Pnt b = cyl.Location();
    base = Base::Vector3d(b.X(), b.Y(), b.Z());
    gp_Dir dir = cyl.Axis().Direction();
    axis = Base::Vector3d(dir.X(), dir.Y(), dir.Z());

    return true;
}

Base::Vector3d Constraint::getBasePoint(const Base::Vector3d& base, const Base::Vector3d& axis,
                                        const App::PropertyLinkSub& location, const double& dist)
{
    // Get the point specified by Location and Distance
    App::DocumentObject* objLoc = location.getValue();
    std::vector<std::string> names = location.getSubValues();
    if (names.size() == 0)
        return Base::Vector3d(0,0,0);
    std::string subName = names.front();
    Part::Feature* featLoc = static_cast<Part::Feature*>(objLoc);
    TopoDS_Shape shloc = featLoc->Shape.getShape().getSubShape(subName.c_str());

    // Get a plane from the Location reference
    gp_Pln plane;
    gp_Dir cylaxis(axis.x, axis.y, axis.z);
    if (shloc.ShapeType() == TopAbs_FACE) {
        BRepAdaptor_Surface surface(TopoDS::Face(shloc));
        plane = surface.Plane();
    } else {
        BRepAdaptor_Curve curve(TopoDS::Edge(shloc));
        gp_Lin line = curve.Line();
        gp_Dir tang = line.Direction().Crossed(cylaxis);
        gp_Dir norm = line.Direction().Crossed(tang);
        plane = gp_Pln(line.Location(), norm);
    }

    // Translate the plane in direction of the cylinder (for positive values of Distance)
    Handle_Geom_Plane pln = new Geom_Plane(plane);
    gp_Pnt cylbase(base.x, base.y, base.z);
    GeomAPI_ProjectPointOnSurf proj(cylbase, pln);
    if (!proj.IsDone())
        return Base::Vector3d(0,0,0);

    gp_Pnt projPnt = proj.NearestPoint();
    if ((fabs(dist) > Precision::Confusion()) && (projPnt.IsEqual(cylbase, Precision::Confusion()) == Standard_False))
        plane.Translate(gp_Vec(projPnt, cylbase).Normalized().Multiplied(dist));
    Handle_Geom_Plane plnt = new Geom_Plane(plane);

    // Intersect translated plane with cylinder axis
    Handle_Geom_Curve crv = new Geom_Line(cylbase, cylaxis);
    GeomAPI_IntCS intersector(crv, plnt);
    if (!intersector.IsDone())
        return Base::Vector3d(0,0,0);
    gp_Pnt inter = intersector.Point(1);
    return Base::Vector3d(inter.X(), inter.Y(), inter.Z());
}

const Base::Vector3d Constraint::getDirection(const App::PropertyLinkSub &direction)
{
    App::DocumentObject* obj = direction.getValue();
    std::vector<std::string> names = direction.getSubValues();
    if (names.size() == 0)
        return Base::Vector3d(0,0,0);
    std::string subName = names.front();
    Part::Feature* feat = static_cast<Part::Feature*>(obj);
    TopoDS_Shape sh = feat->Shape.getShape().getSubShape(subName.c_str());
    gp_Dir dir;

    if (sh.ShapeType() == TopAbs_FACE) {
        BRepAdaptor_Surface surface(TopoDS::Face(sh));
        if (surface.GetType() == GeomAbs_Plane) {
            dir = surface.Plane().Axis().Direction();
        } else {
            return Base::Vector3d(0,0,0); // "Direction must be a planar face or linear edge"
        }
    } else if (sh.ShapeType() == TopAbs_EDGE) {
        BRepAdaptor_Curve line(TopoDS::Edge(sh));
        if (line.GetType() == GeomAbs_Line) {
            dir = line.Line().Direction();
        } else {
            return Base::Vector3d(0,0,0); // "Direction must be a planar face or linear edge"
        }
    }

    Base::Vector3d the_direction(dir.X(), dir.Y(), dir.Z());
    the_direction.Normalize();
    return the_direction;
}
