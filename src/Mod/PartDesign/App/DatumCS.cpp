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
#include "DatumLine.h"
#include <App/Plane.h>
#include <App/Part.h>
#include <App/Line.h>
#include <Base/Exception.h>
#include <gp_Pln.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopoDS.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Geom_Plane.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <GeomAbs_CurveType.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <gp_Quaternion.hxx>
#include <TopoDS_Vertex.hxx>
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
    
    //Point first ----------------------
    key.insert(POINT);
    value.insert(PLANE);
    value.insert(LINE);
    value.insert(POINT);
    value.insert(DONE);
    hints[key] = value;  
    
    key.clear(); value.clear();
    key.insert(POINT);
    key.insert(PLANE);
    value.insert(LINE);
    value.insert(POINT);
    value.insert(DONE);
    hints[key] = value;
    
    key.clear(); value.clear();
    key.insert(POINT);
    key.insert(PLANE);
    key.insert(LINE);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(POINT);
    key.insert(PLANE);
    key.insert(POINT);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(POINT);
    key.insert(LINE);
    value.insert(PLANE);
    value.insert(LINE);
    value.insert(POINT);
    value.insert(DONE);
    hints[key] = value;
    
    key.clear(); value.clear();
    key.insert(POINT);
    key.insert(LINE);
    key.insert(PLANE);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(POINT);
    key.insert(LINE);
    key.insert(LINE);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(POINT);
    key.insert(LINE);
    key.insert(POINT);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(POINT);
    key.insert(POINT);
    value.insert(PLANE);
    value.insert(LINE);
    value.insert(POINT);
    value.insert(DONE);
    hints[key] = value;
    
    key.clear(); value.clear();
    key.insert(POINT);
    key.insert(POINT);
    key.insert(PLANE);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(POINT);
    key.insert(POINT);
    key.insert(LINE);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(POINT);
    key.insert(POINT);
    key.insert(POINT);
    hints[key] = Done;
   
    //Plane First -------------------------
    key.clear(); value.clear();
    key.insert(PLANE);
    value.insert(LINE);
    value.insert(POINT);
    value.insert(DONE);
    hints[key] = value;
    
    key.clear(); value.clear();
    key.insert(PLANE);
    key.insert(LINE);
    value.insert(POINT);
    value.insert(DONE);
    hints[key] = value;
        
    key.clear(); value.clear();
    key.insert(PLANE);
    key.insert(LINE);
    key.insert(POINT);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(PLANE);
    key.insert(POINT);
    value.insert(LINE);
    value.insert(POINT);
    value.insert(DONE);
    hints[key] = value;
    
    key.clear(); value.clear();
    key.insert(PLANE);
    key.insert(POINT);
    key.insert(LINE);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(PLANE);
    key.insert(POINT);
    key.insert(POINT);
    hints[key] = Done;
    
    
    //Line First -------------------------
    key.clear(); value.clear();
    key.insert(LINE);
    value.insert(PLANE);
    value.insert(LINE);
    value.insert(POINT);
    value.insert(DONE);
    hints[key] = value;
    
    key.clear(); value.clear();
    key.insert(LINE);
    key.insert(PLANE);
    value.insert(POINT);
    value.insert(DONE);
    hints[key] = value;
       
    key.clear(); value.clear();
    key.insert(LINE);
    key.insert(PLANE);
    key.insert(POINT);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(LINE);
    key.insert(LINE);
    value.insert(POINT);
    value.insert(DONE);
    hints[key] = value;
    
    key.clear(); value.clear();
    key.insert(LINE);
    key.insert(LINE);
    key.insert(POINT);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(LINE);
    key.insert(POINT);
    value.insert(PLANE);
    value.insert(LINE);
    value.insert(POINT);
    value.insert(DONE);
    hints[key] = value;
    
    key.clear(); value.clear();
    key.insert(LINE);
    key.insert(POINT);
    key.insert(PLANE);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(LINE);
    key.insert(POINT);
    key.insert(POINT);
    hints[key] = Done;
    
    key.clear(); value.clear();
    key.insert(LINE);
    key.insert(PLANE);
    key.insert(POINT);
    hints[key] = Done;
}

// ============================================================================

#define GP_POINT(p) \
    gp_Pnt(p[0], p[1], p[2])

#define GP_DIR(p) \
    gp_Dir(p[0], p[1], p[2])
    
    
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
            throw Base::Exception("Size of references and subreferences do not match");
        
        refTypes.clear();
        for (int r = 0; r < refs.size(); r++)
            refTypes.insert(getRefType(refs[r], subrefs[r]));

        std::set<QString> hint = getHint();
        if (refs.size() != 0 && !(hint.find(QObject::tr("Done")) != hint.end()))
            throw Base::Exception("Can not build coordinate system from given references"); // incomplete references
            
        //build the placement from the references
        bool plane = false, line1 = false, line2 = false, point1 = false, point2=false, point3=false;
        gp_Pln pln;
        gp_Lin lin1, lin2;
        gp_Pnt p1,  p2, p3;
        
        int count = 0;
        for(int i = 0; i < refs.size(); i++) {
            
            if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())) {
                PartDesign::Point* p = static_cast<PartDesign::Point*>(refs[i]);
                if(!point1) {
                    p1 = GP_POINT(p->getPoint());
                    point1=true;
                } else if(!point2) {
                    p2 = GP_POINT(p->getPoint());
                    point2=true;
                } else if(!point3) {
                    p3 = GP_POINT(p->getPoint());
                    point3=true;
                }
                else 
                    throw Base::Exception("Too many points in coordinate system references"); //too much points selected
            }
            else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
                PartDesign::Plane* p = static_cast<PartDesign::Plane*>(refs[i]);
                if(!plane) {
                    pln = gp_Pln(GP_POINT(p->getBasePoint()), GP_DIR(p->getNormal()));
                    plane=true;
                }
                else 
                    throw Base::Exception("Too many planes in coordinate syste references");
            } else if (refs[i]->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                App::Plane* p = static_cast<App::Plane*>(refs[i]);
                if(!plane) {
                    gp_Pnt base(0,0,0);
                    gp_Dir dir(0,0,1);
                    if (strcmp(p->PlaneType.getValue(), App::Part::BaseplaneTypes[0]) == 0)
                        dir = gp_Dir(0,0,1);
                    else if (strcmp(p->PlaneType.getValue(), App::Part::BaseplaneTypes[1]) == 0)
                        dir = gp_Dir(0,1,0);
                    else if (strcmp(p->PlaneType.getValue(), App::Part::BaseplaneTypes[2]) == 0)
                        dir = gp_Dir(1,0,0);
                    
                    pln = gp_Pln(base, dir);
                    plane=true;
                }
                else 
                    throw Base::Exception("Too many planes in coordinate syste references"); //too much planes selected
            }
            else if (refs[i]->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())) {
                PartDesign::Line* p = static_cast<PartDesign::Line*>(refs[i]);
                if(!line1) {
                    lin1 = gp_Lin(GP_POINT(p->getBasePoint()), GP_DIR(p->getDirection()));
                    line1 = true;
                }
                else if(!line2) {
                    lin2 = gp_Lin(GP_POINT(p->getBasePoint()), GP_DIR(p->getDirection()));
                    line2 = true;
                }
                else 
                    throw Base::Exception("Too many lines in coordinate syste references");; //too much lines selected
                
            } else if (refs[i]->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
                App::Line* p = static_cast<App::Line*>(refs[i]);
                gp_Pnt base(0,0,0);
                gp_Dir dir(0,0,1);
                if (strcmp(p->LineType.getValue(), App::Part::BaselineTypes[0]) == 0)
                    dir = gp_Dir(1,0,0);
                else if (strcmp(p->LineType.getValue(), App::Part::BaselineTypes[1]) == 0)
                    dir = gp_Dir(0,1,0);
                else if (strcmp(p->LineType.getValue(), App::Part::BaselineTypes[2]) == 0)
                    dir = gp_Dir(0,0,1);
                
                if(!line1) {    
                    lin1 = gp_Lin(base, dir);
                    line1=true;
                }
                else if(!line2) {    
                    lin2 = gp_Lin(base, dir);
                    line2=true;
                }
                else 
                    throw Base::Exception("Too many lines in coordinate syste references");
            }
            else if (refs[i]->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                Part::Feature* feature = static_cast<Part::Feature*>(refs[i]);
                const TopoDS_Shape& sh = feature->Shape.getValue();
                if (sh.IsNull())
                    throw Base::Exception("Invalid shape in reference");
                // Get subshape
                TopoDS_Shape subshape = feature->Shape.getShape().getSubShape(subrefs[i].c_str());
                if (subshape.IsNull())
                    throw Base::Exception("Reference has Null shape");

                if (subshape.ShapeType() == TopAbs_VERTEX) {
                    TopoDS_Vertex v = TopoDS::Vertex(subshape);

                    if(!point1) {
                        p1 = BRep_Tool::Pnt(v);
                        point1=true;
                    } else if(!point2) {
                        p2 = BRep_Tool::Pnt(v);
                        point2=true;
                    } else if(!point3) {
                        p3 = BRep_Tool::Pnt(v);
                        point3=true;
                    }
                    else 
                        throw Base::Exception("Too many points in coordinate system references");
                }
                else if (subshape.ShapeType() == TopAbs_EDGE) { 
                    TopoDS_Edge e = TopoDS::Edge(subshape);
                    BRepAdaptor_Curve adapt(e);
                    if (adapt.GetType() != GeomAbs_Line)
                        throw Base::Exception("Only straight edges are supported");
                        
                    if(!line1) {    
                        lin1 = adapt.Line();
                        line1=true;
                    }
                    else if(!line2) {    
                        lin2 = adapt.Line();
                        line2=true;
                    }
                    else 
                        throw Base::Exception("Too many lines in coordinate system references");           
                        
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
                       
                        if(!plane) {
                            pln = gp_Pln(p, d);
                            plane     = true;
                        }
                        else 
                            throw Base::Exception("Too many planes in coordinate system references");
                            
                    } else {
                        throw Base::Exception("Only planar faces allowed");
                    }
                }
            }
            
            count++;
        };
        
        gp_Ax3 ax;
        if(point1) {            
            
            if(plane) {
                if(!pln.Contains(p1, Precision::Confusion()))
                    throw Base::Exception("Point must lie on plane");
                    
                if(point2) {
                    if(!pln.Contains(p2, Precision::Confusion()))
                        throw Base::Exception("Point must lie on plane");
                    
                    if(p1.Distance(p2) < Precision::Confusion())
                        throw Base::Exception("Point must not be coincident");
                    
                    ax = gp_Ax3(p1, pln.Axis().Direction(), gp_Dir((p2.XYZ()-p1.XYZ())));
                }
                else if(line1) {
                    if(!pln.Contains(lin1, Precision::Confusion(), Precision::Confusion()))
                        throw Base::Exception("Line must lie on plane");
                    
                    if(line2)
                        throw Base::Exception("Two lines and a plain are not supportet");
                    
                    ax = gp_Ax3(p1, pln.Axis().Direction(), lin1.Direction());         
                }
                else {
                    ax = gp_Ax3(p1, pln.Axis().Direction(), pln.XAxis().Direction());
                }
            }
            else if(line1) {
                
                if(point2) {
                    if(!gp_Lin(p1, lin1.Direction()).Contains(p2, Precision::Confusion()))
                        throw Base::Exception("Second Point must not lie on z-axis");
                    
                    if(p1.Distance(p2) < Precision::Confusion())
                        throw Base::Exception("Points must not be coincident");
                    
                    ax = gp_Ax3(p1, lin1.Direction(), gp_Dir((p2.XYZ()-p1.XYZ())));
                } 
                else if(line2) {
                    if(lin2.Angle(lin1)<Precision::Angular())
                        return;
                    
                    ax = gp_Ax3(p1, lin1.Direction(), lin2.Direction());
                }
                else 
                    ax = gp_Ax3(p1, lin1.Direction());
            }
            else if(point2) {
             
                if(p1.Distance(p2) < Precision::Confusion())
                        throw Base::Exception("Points must not be coincident");
                
                if(point3) {
                    if(p1.Distance(p3) < 2*Precision::Confusion())
                        throw Base::Exception("Points must not be coincident");
                    
                    ax = gp_Ax3(p1, gp_Dir((p2.XYZ()-p1.XYZ())), gp_Dir((p3.XYZ()-p2.XYZ())));
                }
                else {
                    ax = gp_Ax3(p1, gp_Dir((p2.XYZ()-p1.XYZ())));
                }
            }
            else
                ax = gp_Ax3(p1, gp_Dir(0,0,1), gp_Dir(1,0,0));
        }
        else if(plane) {
                
            if(line1) {
                if(!pln.Contains(lin1, Precision::Confusion(), Precision::Confusion()))
                        throw Base::Exception("Line must lie on plane");
                
                ax = gp_Ax3(pln.Location(), pln.Axis().Direction(), lin1.Direction());        
            }
            else {
                ax = gp_Ax3(pln.Location(), pln.Axis().Direction(), pln.XAxis().Direction());
            }            
        }
        else if(line1) {
            
            if(line2) {
                if(! (fabs(lin1.Angle(lin2) - M_PI/2.0) < Precision::Angular()) )
                        throw Base::Exception("Lines must be perpendicular");
                ax = gp_Ax3(lin1.Location(), lin1.Direction(), lin2.Direction());
            }
            else 
                ax = gp_Ax3(lin1.Location(), lin1.Direction());
        }
            
        //build the placement
        gp_Trsf trans;
        trans.SetTransformation(ax);
        trans.Invert();
        gp_XYZ p = trans.TranslationPart();
        gp_Quaternion q = trans.GetRotation();        
        plm = Base::Placement(Base::Vector3d(p.X(), p.Y(), p.Z()), Base::Rotation(q.X(), q.Y(), q.Z(), q.W()));
        
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
