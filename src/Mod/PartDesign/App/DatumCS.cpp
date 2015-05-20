/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
#endif

#include "DatumCS.h"
#include "DatumPoint.h"
#include "DatumPlane.h"
#include <App/Plane.h>
#include <App/Part.h>
#include <gp_Pln.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopoDS.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Geom_Plane.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <QObject>

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
#define CS QObject::tr("DCOORDINATESYSTEM")
#define DONE QObject::tr("Done")

std::map<std::multiset<QString>, std::set<QString> > CoordinateSystem::hints = std::map<std::multiset<QString>, std::set<QString> >();

void CoordinateSystem::initHints()
{ 
    std::set<QString> Done;
    Done.insert(QObject::tr("Done"));

    std::multiset<QString> key;
    std::set<QString> value;
    key.insert(POINT);
    hints[key] = Done;  
    
    key.clear(); value.clear();
    key.insert(PLANE);
    value.insert(LINE);
    value.insert(DONE);
    hints[key] = value;
    
    key.clear(); value.clear();
    key.insert(PLANE);
    key.insert(LINE);
    value.insert(LINE);
    value.insert(DONE);
    hints[key] = value;
}

// ============================================================================

PROPERTY_SOURCE(PartDesign::CoordinateSystem, Part::Datum)

CoordinateSystem::CoordinateSystem()
{
    // Create a shape, which will be used by the Sketcher. Them main function is to avoid a dependency of
    // Sketcher on the PartDesign module
    BRepBuilderAPI_MakeFace builder(gp_Pln(gp_Pnt(0,0,0), gp_Dir(0,0,1)));
    if (!builder.IsDone())
        return;
    Shape.setValue(builder.Shape());

    References.touch();
}

CoordinateSystem::~CoordinateSystem()
{
}

void CoordinateSystem::onChanged(const App::Property *prop)
{
    if ((prop == &References) || (prop == &Offset) || (prop == &Offset2) || (prop == &Offset3)) {
        
        Base::Placement plm;
        const std::vector<App::DocumentObject*>& refs    = References.getValues();
        const std::vector<std::string>&          subrefs = References.getSubValues();
        
        if (refs.size() != subrefs.size())
            return;
        
        refTypes.clear();
        for (int r = 0; r < refs.size(); r++)
            refTypes.insert(getRefType(refs[r], subrefs[r]));

        std::set<QString> hint = getHint();
        if (refs.size() != 0 && !(hint.find(QObject::tr("Done")) != hint.end()))
            return; // incomplete references
            
        //build the placement from the references
        bool plane = false;
        Base::Vector3d pl_base(0,0,0), pl_normal(0,0,1);
        
        int count = 0;
        for(int i = 0; i < refs.size(); i++) {
            
            if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
                PartDesign::Plane* p = static_cast<PartDesign::Plane*>(refs[i]);
                if(!plane) {
                    pl_base = p->getBasePoint();
                    pl_normal = p->getNormal();
                    plane=true;
                }
            } else if (refs[i]->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                PartDesign::Plane* p = static_cast<PartDesign::Plane*>(refs[i]);
                if(!plane) {
                    pl_base = Base::Vector3d(0,0,0);
                    if (strcmp(p->getNameInDocument(), App::Part::BaseplaneTypes[0]) == 0)
                        pl_normal = Base::Vector3d(0,0,1);
                    else if (strcmp(p->getNameInDocument(), App::Part::BaseplaneTypes[2]) == 0)
                        pl_normal = Base::Vector3d(1,0,0);
                    else if (strcmp(p->getNameInDocument(), App::Part::BaseplaneTypes[1]) == 0)
                        pl_normal = Base::Vector3d(0,1,0);
                    
                    plane=true;
                }
            }
            else if (refs[i]->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                Part::Feature* feature = static_cast<Part::Feature*>(refs[i]);
                const TopoDS_Shape& sh = feature->Shape.getValue();
                if (sh.IsNull())
                    return; // "PartDesign::Plane: Reference has NULL shape"
                // Get subshape
                TopoDS_Shape subshape = feature->Shape.getShape().getSubShape(subrefs[i].c_str());
                if (subshape.IsNull())
                    return; // "PartDesign::Plane: Reference has NULL subshape";

                if (subshape.ShapeType() == TopAbs_VERTEX) {/*
                    TopoDS_Vertex v = TopoDS::Vertex(subshape);
                    gp_Pnt p = BRep_Tool::Pnt(v);
                    if (p1 == NULL)
                        p1 = new Base::Vector3d(p.X(), p.Y(), p.Z());
                    else if (p2 == NULL)
                        p2 = new Base::Vector3d(p.X(), p.Y(), p.Z());
                    else
                        p3 = new Base::Vector3d(p.X(), p.Y(), p.Z());*/
                } else if (subshape.ShapeType() == TopAbs_EDGE) { /*
                    TopoDS_Edge e = TopoDS::Edge(subshape);
                    BRepAdaptor_Curve adapt(e);
                    if (adapt.GetType() != GeomAbs_Line)
                        return; // Non-linear edge
                    line = new gp_Lin(adapt.Line());*/
                } else if (subshape.ShapeType() == TopAbs_FACE) {
                    TopoDS_Face f = TopoDS::Face(subshape);
                    BRepAdaptor_Surface adapt(f);
                    if (adapt.GetType() == GeomAbs_Plane) {
                        // Ensure that the front and back of the plane corresponds with the face's idea of front and back
                        bool reverse = (f.Orientation() == TopAbs_REVERSED);
                        gp_Pln pl = adapt.Plane();
                        gp_Dir d = pl.Axis().Direction();
                        const gp_Pnt& p = pl.Location();
                        if (reverse) d.Reverse();
                       
                        pl_base   = Base::Vector3d(p.X(), p.Y(), p.Z());
                        pl_normal = Base::Vector3d(d.X(), d.Y(), d.Z());
                        plane     = true;
                    } else {
                        return; // invalid surface type
                    }
                }
            }
            
            count++;
        };
        
        if(plane) {
            plm = Base::Placement(pl_base, Base::Rotation(Base::Vector3d(0,0,1), pl_normal));            
        }
            
        //add the offsets 
        Base::Vector3d o1;
        plm.getRotation().multVec(Base::Vector3d(1,0,0), o1);
        Base::Vector3d o2;
        plm.getRotation().multVec(Base::Vector3d(0,1,0), o2);
        Base::Vector3d o3;
        plm.getRotation().multVec(Base::Vector3d(0,0,1), o3);
        plm.move(Offset.getValue()*o1+Offset2.getValue()*o2+Offset3.getValue()*o3);

        Placement.setValue(plm);
    }
    Part::Datum::onChanged(prop);
}


const std::set<QString> CoordinateSystem::getHint() const
{
    if (hints.find(refTypes) != hints.end())
        return hints[refTypes];
    else
        return std::set<QString>();
}

const int CoordinateSystem::offsetsAllowed() const
{
    return 3;
}

Base::Vector3d CoordinateSystem::getXAxis()
{
    Base::Rotation rot = Placement.getValue().getRotation();
    Base::Vector3d normal;
    rot.multVec(Base::Vector3d(1,0,0), normal);
    return normal;
}

Base::Vector3d CoordinateSystem::getYAxis()
{
    Base::Rotation rot = Placement.getValue().getRotation();
    Base::Vector3d normal;
    rot.multVec(Base::Vector3d(0,1,0), normal);
    return normal;
}
 
Base::Vector3d CoordinateSystem::getZAxis()
{
    Base::Rotation rot = Placement.getValue().getRotation();
    Base::Vector3d normal;
    rot.multVec(Base::Vector3d(0,0,1), normal);
    return normal;
}