/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepPrimAPI_MakeCone.hxx>
# include <BRepPrimAPI_MakeCylinder.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepPrimAPI_MakeSphere.hxx>
# include <BRepPrimAPI_MakeTorus.hxx>
# include <BRepPrim_Wedge.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_GTransform.hxx>
# include <gp_Circ.hxx>
# include <gp_GTrsf.hxx>
# include <GCE2d_MakeSegment.hxx>
# include <Geom_Plane.hxx>
# include <Geom_ConicalSurface.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom2d_Line.hxx>
# include <Geom2d_TrimmedCurve.hxx>
# include <Handle_Geom_Plane.hxx>
# include <Handle_Geom_CylindricalSurface.hxx>
# include <Handle_Geom2d_Line.hxx>
# include <Handle_Geom2d_TrimmedCurve.hxx>
# include <Precision.hxx>
# include <Standard_Real.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Vertex.hxx>
# include <Standard_Version.hxx>
#endif


#include "PrimitiveFeature.h"
#include <Base/Tools.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif


namespace Part {
    const App::PropertyFloatConstraint::Constraints floatRange  = {0.0f,FLT_MAX,0.1f};
    const App::PropertyFloatConstraint::Constraints apexRange   = {0.0f,90.0f,0.1f};
    const App::PropertyFloatConstraint::Constraints angleRangeU = {0.0f,360.0f,1.0f};
    const App::PropertyFloatConstraint::Constraints angleRangeV = {-90.0f,90.0f,1.0f};
    const App::PropertyFloatConstraint::Constraints torusRangeV = {-180.0f,180.0f,1.0f};
}

using namespace Part;


PROPERTY_SOURCE_ABSTRACT(Part::Primitive, Part::Feature)

Primitive::Primitive(void) 
{
    touch();
}

Primitive::~Primitive()
{
}

short Primitive::mustExecute(void) const
{
    return Feature::mustExecute();
}

void Primitive::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        // Do not support sphere, ellipsoid and torus because the creation
        // takes too long and thus is not feasible
        std::string grp = (prop->getGroup() ? prop->getGroup() : "");
        if (grp == "Plane" || grp == "Cylinder" || grp == "Cone"){
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    Part::Feature::onChanged(prop);
}

PROPERTY_SOURCE(Part::Vertex, Part::Primitive)

Vertex::Vertex()
{
    ADD_PROPERTY(X,(0.0f));
    ADD_PROPERTY(Y,(0.0f));
    ADD_PROPERTY(Z,(0.0f));
}

Vertex::~Vertex()
{
}

short Vertex::mustExecute() const
{
    if (X.isTouched() ||
        Y.isTouched() ||
        Z.isTouched())
        return 1;
    return Part::Feature::mustExecute();
}

App::DocumentObjectExecReturn *Vertex::execute(void)
{
    gp_Pnt point;
    point.SetX(this->X.getValue());
    point.SetY(this->Y.getValue());
    point.SetZ(this->Z.getValue());
    
    BRepBuilderAPI_MakeVertex MakeVertex(point);
    const TopoDS_Vertex& vertex = MakeVertex.Vertex();
    this->Shape.setValue(vertex);

    return App::DocumentObject::StdReturn;
}


void Vertex::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &X || prop == &Y || prop == &Z){
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    Part::Feature::onChanged(prop);
}

PROPERTY_SOURCE(Part::Line, Part::Primitive)

Line::Line()
{
    ADD_PROPERTY_TYPE(X1,(0.0f),"Vertex 1 - Start",App::Prop_None,"X value of the start vertex");
    ADD_PROPERTY_TYPE(Y1,(0.0f),"Vertex 1 - Start",App::Prop_None,"Y value of the Start vertex");
    ADD_PROPERTY_TYPE(Z1,(0.0f),"Vertex 1 - Start",App::Prop_None,"Z value of the Start vertex");
    ADD_PROPERTY_TYPE(X2,(0.0f),"Vertex 2 - Finish",App::Prop_None,"X value of the finish vertex");
    ADD_PROPERTY_TYPE(Y2,(0.0f),"Vertex 2 - Finish",App::Prop_None,"Y value of the finish vertex");
    ADD_PROPERTY_TYPE(Z2,(1.0f),"Vertex 2 - Finish",App::Prop_None,"Z value of the finish vertex");
}

Line::~Line()
{
}

short Line::mustExecute() const
{
    if (X1.isTouched() ||
        Y1.isTouched() ||
        Z1.isTouched() ||
        X2.isTouched() ||
        Y2.isTouched() ||
        Z2.isTouched())
        return 1;
    return Part::Feature::mustExecute();
}

App::DocumentObjectExecReturn *Line::execute(void)
{
    gp_Pnt point1;
    point1.SetX(this->X1.getValue());
    point1.SetY(this->Y1.getValue());
    point1.SetZ(this->Z1.getValue());

    gp_Pnt point2;
    point2.SetX(this->X2.getValue());
    point2.SetY(this->Y2.getValue());
    point2.SetZ(this->Z2.getValue());

    BRepBuilderAPI_MakeEdge mkEdge(point1, point2);
    if (!mkEdge.IsDone())
        return new App::DocumentObjectExecReturn("Failed to create edge");
    const TopoDS_Edge& edge = mkEdge.Edge();
    this->Shape.setValue(edge);

    return App::DocumentObject::StdReturn;
}

void Line::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &X1 || prop == &Y1 || prop == &Z1 || prop == &X2 || prop == &Y2 || prop == &Z2){
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    Part::Feature::onChanged(prop);
}

PROPERTY_SOURCE(Part::Plane, Part::Primitive)

Plane::Plane()
{
    ADD_PROPERTY_TYPE(Length,(100.0f),"Plane",App::Prop_None,"The length of the plane");
    ADD_PROPERTY_TYPE(Width ,(100.0f),"Plane",App::Prop_None,"The width of the plane");
}

short Plane::mustExecute() const
{
    if (Length.isTouched() ||
        Width.isTouched() )
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Plane::execute(void)
{
    double L = this->Length.getValue();
    double W = this->Width.getValue();

    if (L < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Length of plane too small");
    if (W < Precision::Confusion())
      return new App::DocumentObjectExecReturn("Width of plane too small");

    gp_Pnt pnt(0.0,0.0,0.0);
    gp_Dir dir(0.0,0.0,1.0);
    Handle_Geom_Plane aPlane = new Geom_Plane(pnt, dir);
    BRepBuilderAPI_MakeFace mkFace(aPlane, 0.0, L, 0.0, W
#if OCC_VERSION_HEX >= 0x060502
      , Precision::Confusion()
#endif
    );

    const char *error=0;
    switch (mkFace.Error())
    {
    case BRepBuilderAPI_FaceDone:
        break; // ok
    case BRepBuilderAPI_NoFace:
        error = "no face";
        break;
    case BRepBuilderAPI_NotPlanar:
        error = "not planar";
        break;
    case BRepBuilderAPI_CurveProjectionFailed:
        break;
    case BRepBuilderAPI_ParametersOutOfRange:
        error = "parameters out of range";
        break;
#if OCC_HEX_VERSION < 0x060500
    case BRepBuilderAPI_SurfaceNotC2:
        error = "surface not C2";
        break;
#endif
    default:
        error = "unknown error";
        break;
    }
    // Error ?
    if (error) {
        return new App::DocumentObjectExecReturn(error);
    }

    TopoDS_Shape ResultShape = mkFace.Shape();
    this->Shape.setValue(ResultShape);

    return App::DocumentObject::StdReturn;
}

PROPERTY_SOURCE(Part::Sphere, Part::Primitive)

Sphere::Sphere(void)
{
    ADD_PROPERTY_TYPE(Radius,(5.0),"Sphere",App::Prop_None,"The radius of the sphere");
    Radius.setConstraints(&floatRange);
    ADD_PROPERTY_TYPE(Angle1,(-90.0f),"Sphere",App::Prop_None,"The angle of the sphere");
    Angle1.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle2,(90.0f),"Sphere",App::Prop_None,"The angle of the sphere");
    Angle2.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle3,(360.0f),"Sphere",App::Prop_None,"The angle of the sphere");
    Angle3.setConstraints(&angleRangeU);
}

short Sphere::mustExecute() const
{
    if (Radius.isTouched())
        return 1;
    if (Angle1.isTouched())
        return 1;
    if (Angle2.isTouched())
        return 1;
    if (Angle3.isTouched())
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Sphere::execute(void)
{
    // Build a sphere
    if (Radius.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Radius of sphere too small");
    try {
        BRepPrimAPI_MakeSphere mkSphere(Radius.getValue(),
                                        Angle1.getValue()/180.0f*Standard_PI,
                                        Angle2.getValue()/180.0f*Standard_PI,
                                        Angle3.getValue()/180.0f*Standard_PI);
        TopoDS_Shape ResultShape = mkSphere.Shape();
        this->Shape.setValue(ResultShape);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

PROPERTY_SOURCE(Part::Ellipsoid, Part::Primitive)

Ellipsoid::Ellipsoid(void)
{
    ADD_PROPERTY_TYPE(Radius1,(2.0),"Ellipsoid",App::Prop_None,"The radius of the ellipsoid");
    Radius1.setConstraints(&floatRange);
    ADD_PROPERTY_TYPE(Radius2,(4.0),"Ellipsoid",App::Prop_None,"The radius of the ellipsoid");
    Radius2.setConstraints(&floatRange);
    ADD_PROPERTY_TYPE(Angle1,(-90.0f),"Ellipsoid",App::Prop_None,"The angle of the ellipsoid");
    Angle1.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle2,(90.0f),"Ellipsoid",App::Prop_None,"The angle of the ellipsoid");
    Angle2.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle3,(360.0f),"Ellipsoid",App::Prop_None,"The angle of the ellipsoid");
    Angle3.setConstraints(&angleRangeU);
}

short Ellipsoid::mustExecute() const
{
    if (Radius1.isTouched())
        return 1;
    if (Radius2.isTouched())
        return 1;
    if (Angle1.isTouched())
        return 1;
    if (Angle2.isTouched())
        return 1;
    if (Angle3.isTouched())
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Ellipsoid::execute(void)
{
    // Build a sphere
    if (Radius1.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Radius of ellipsoid too small");
    if (Radius2.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Radius of ellipsoid too small");
    try {
        gp_Pnt pnt(0.0,0.0,0.0);
        gp_Dir dir(0.0,0.0,1.0);
        gp_Ax2 ax2(pnt,dir);
        BRepPrimAPI_MakeSphere mkSphere(ax2,
                                        Radius2.getValue(), 
                                        Angle1.getValue()/180.0f*Standard_PI,
                                        Angle2.getValue()/180.0f*Standard_PI,
                                        Angle3.getValue()/180.0f*Standard_PI);
        Standard_Real scale = Radius1.getValue()/Radius2.getValue();
        gp_Dir xDir = ax2.XDirection();
        gp_Dir yDir = ax2.YDirection();
        gp_GTrsf mat;
        mat.SetValue(1,1,xDir.X());
        mat.SetValue(2,1,xDir.Y());
        mat.SetValue(3,1,xDir.Z());
        mat.SetValue(1,2,yDir.X());
        mat.SetValue(2,2,yDir.Y());
        mat.SetValue(3,2,yDir.Z());
        mat.SetValue(1,3,dir.X()*scale);
        mat.SetValue(2,3,dir.Y()*scale);
        mat.SetValue(3,3,dir.Z()*scale);
        BRepBuilderAPI_GTransform mkTrsf(mkSphere.Shape(), mat);
        TopoDS_Shape ResultShape = mkTrsf.Shape();
        this->Shape.setValue(ResultShape);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

PROPERTY_SOURCE(Part::Cylinder, Part::Primitive)

Cylinder::Cylinder(void)
{
    ADD_PROPERTY_TYPE(Radius,(2.0),"Cylinder",App::Prop_None,"The radius of the cylinder");
    ADD_PROPERTY_TYPE(Height,(10.0f),"Cylinder",App::Prop_None,"The height of the cylinder");
    ADD_PROPERTY_TYPE(Angle,(360.0f),"Cylinder",App::Prop_None,"The angle of the cylinder");
    Angle.setConstraints(&angleRangeU);
}

short Cylinder::mustExecute() const
{
    if (Radius.isTouched())
        return 1;
    if (Height.isTouched())
        return 1;
    if (Angle.isTouched())
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Cylinder::execute(void)
{
    // Build a cylinder
    if (Radius.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Radius of cylinder too small");
    if (Height.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Height of cylinder too small");
    try {
        BRepPrimAPI_MakeCylinder mkCylr(Radius.getValue(),
                                        Height.getValue(),
                                        Angle.getValue()/180.0f*Standard_PI);
        TopoDS_Shape ResultShape = mkCylr.Shape();
        this->Shape.setValue(ResultShape);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

PROPERTY_SOURCE(Part::Cone, Part::Primitive)

Cone::Cone(void)
{
    ADD_PROPERTY_TYPE(Radius1,(2.0),"Cone",App::Prop_None,"The radius of the cone");
    ADD_PROPERTY_TYPE(Radius2,(4.0),"Cone",App::Prop_None,"The radius of the cone");
    ADD_PROPERTY_TYPE(Height,(10.0),"Cone",App::Prop_None,"The height of the cone");
    ADD_PROPERTY_TYPE(Angle,(360.0),"Cone",App::Prop_None,"The angle of the cone");
    Angle.setConstraints(&angleRangeU);
}

short Cone::mustExecute() const
{
    if (Radius1.isTouched())
        return 1;
    if (Radius2.isTouched())
        return 1;
    if (Height.isTouched())
        return 1;
    if (Angle.isTouched())
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Cone::execute(void)
{
    if (Radius1.getValue() < 0)
        return new App::DocumentObjectExecReturn("Radius of cone too small");
    if (Radius2.getValue() < 0)
        return new App::DocumentObjectExecReturn("Radius of cone too small");
    if (Height.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Height of cone too small");
    try {
        // Build a cone
        BRepPrimAPI_MakeCone mkCone(Radius1.getValue(),
                                    Radius2.getValue(),
                                    Height.getValue(),
                                    Angle.getValue()/180.0f*Standard_PI);
        TopoDS_Shape ResultShape = mkCone.Shape();
        this->Shape.setValue(ResultShape);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

PROPERTY_SOURCE(Part::Torus, Part::Primitive)

Torus::Torus(void)
{
    ADD_PROPERTY_TYPE(Radius1,(10.0),"Torus",App::Prop_None,"The radius of the torus");
    Radius1.setConstraints(&floatRange);
    ADD_PROPERTY_TYPE(Radius2,(2.0),"Torus",App::Prop_None,"The radius of the torus");
    Radius2.setConstraints(&floatRange);
    ADD_PROPERTY_TYPE(Angle1,(-180.0),"Torus",App::Prop_None,"The angle of the torus");
    Angle1.setConstraints(&torusRangeV);
    ADD_PROPERTY_TYPE(Angle2,(180.0),"Torus",App::Prop_None,"The angle of the torus");
    Angle2.setConstraints(&torusRangeV);
    ADD_PROPERTY_TYPE(Angle3,(360.0),"Torus",App::Prop_None,"The angle of the torus");
    Angle3.setConstraints(&angleRangeU);
}

short Torus::mustExecute() const
{
    if (Radius1.isTouched())
        return 1;
    if (Radius2.isTouched())
        return 1;
    if (Angle1.isTouched())
        return 1;
    if (Angle2.isTouched())
        return 1;
    if (Angle3.isTouched())
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Torus::execute(void)
{
    if (Radius1.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Radius of torus too small");
    if (Radius2.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Radius of torus too small");
    try {
#if 1
        // Build a torus
        gp_Circ circle;
        circle.SetRadius(Radius2.getValue());
        gp_Pnt pos(Radius1.getValue(),0,0);
        gp_Dir dir(0,1,0);
        circle.SetAxis(gp_Ax1(pos, dir));

        BRepBuilderAPI_MakeEdge mkEdge(circle, Base::toRadians<double>(Angle1.getValue()+180.0f),
                                               Base::toRadians<double>(Angle2.getValue()+180.0f));
        BRepBuilderAPI_MakeWire mkWire;
        mkWire.Add(mkEdge.Edge());
        BRepBuilderAPI_MakeFace mkFace(mkWire.Wire());
        BRepPrimAPI_MakeRevol mkRevol(mkFace.Face(), gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,0,1)),
            Base::toRadians<double>(Angle3.getValue()), Standard_True);
        TopoDS_Shape ResultShape = mkRevol.Shape();
#else
        BRepPrimAPI_MakeTorus mkTorus(Radius1.getValue(),
                                      Radius2.getValue(),
                                      Angle1.getValue()/180.0f*Standard_PI,
                                      Angle2.getValue()/180.0f*Standard_PI,
                                      Angle3.getValue()/180.0f*Standard_PI);
        const TopoDS_Solid& ResultShape = mkTorus.Solid();
#endif
        this->Shape.setValue(ResultShape);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

PROPERTY_SOURCE(Part::Helix, Part::Primitive)

Helix::Helix(void)
{
    ADD_PROPERTY_TYPE(Pitch, (1.0),"Helix",App::Prop_None,"The pitch of the helix");
    Pitch.setConstraints(&floatRange);
    ADD_PROPERTY_TYPE(Height,(2.0),"Helix",App::Prop_None,"The height of the helix");
    Height.setConstraints(&floatRange);
    ADD_PROPERTY_TYPE(Radius,(1.0),"Helix",App::Prop_None,"The radius of the helix");
    Radius.setConstraints(&floatRange);
    ADD_PROPERTY_TYPE(Angle,(0.0),"Helix",App::Prop_None,"If angle is > 0 a conical otherwise a cylindircal surface is used");
    Angle.setConstraints(&apexRange);
}

short Helix::mustExecute() const
{
    if (Pitch.isTouched())
        return 1;
    if (Height.isTouched())
        return 1;
    if (Radius.isTouched())
        return 1;
    if (Angle.isTouched())
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Helix::execute(void)
{
    try {
        Standard_Real myPitch  = Pitch.getValue();
        Standard_Real myHeight = Height.getValue();
        Standard_Real myRadius = Radius.getValue();
        Standard_Real myAngle  = Angle.getValue();
        TopoShape helix;
        this->Shape.setValue(helix.makeHelix(myPitch, myHeight, myRadius, myAngle));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

PROPERTY_SOURCE(Part::Wedge, Part::Primitive)

Wedge::Wedge()
{
    ADD_PROPERTY_TYPE(Xmin,(0.0f),"Wedge",App::Prop_None,"Xmin of the wedge");
    ADD_PROPERTY_TYPE(Ymin,(0.0f),"Wedge",App::Prop_None,"Ymin of the wedge");
    ADD_PROPERTY_TYPE(Zmin,(0.0f),"Wedge",App::Prop_None,"Zmin of the wedge");
    ADD_PROPERTY_TYPE(X2min,(2.0f),"Wedge",App::Prop_None,"X2min of the wedge");
    ADD_PROPERTY_TYPE(Z2min,(2.0f),"Wedge",App::Prop_None,"Z2min of the wedge");
    ADD_PROPERTY_TYPE(Xmax,(10.0f),"Wedge",App::Prop_None,"Xmax of the wedge");
    ADD_PROPERTY_TYPE(Ymax,(10.0f),"Wedge",App::Prop_None,"Ymax of the wedge");
    ADD_PROPERTY_TYPE(Zmax,(10.0f),"Wedge",App::Prop_None,"Zmax of the wedge");
    ADD_PROPERTY_TYPE(X2max,(8.0f),"Wedge",App::Prop_None,"X2max of the wedge");
    ADD_PROPERTY_TYPE(Z2max,(8.0f),"Wedge",App::Prop_None,"Z2max of the wedge");
}

short Wedge::mustExecute() const
{
    if (Xmin.isTouched() ||
        Ymin.isTouched() ||
        Zmin.isTouched() ||
        X2min.isTouched() ||
        Z2min.isTouched() ||
        Xmax.isTouched() ||
        Ymax.isTouched() ||
        Zmax.isTouched() ||
        X2max.isTouched() ||
        Z2max.isTouched())
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Wedge::execute(void)
{
    double xmin = Xmin.getValue();
    double ymin = Ymin.getValue();
    double zmin = Zmin.getValue();
    double z2min = Z2min.getValue();
    double x2min = X2min.getValue();
    double xmax = Xmax.getValue();
    double ymax = Ymax.getValue();
    double zmax = Zmax.getValue();
    double z2max = Z2max.getValue();
    double x2max = X2max.getValue();


    double dx = xmax-xmin;
    double dy = ymax-ymin;
    double dz = zmax-zmin;
    double dz2 = z2max-z2min;
    double dx2 = x2max-x2min;

    if (dx < Precision::Confusion())
        return new App::DocumentObjectExecReturn("delta x of wedge too small");

    if (dy < Precision::Confusion())
        return new App::DocumentObjectExecReturn("delta y of wedge too small");

    if (dz < Precision::Confusion())
        return new App::DocumentObjectExecReturn("delta z of wedge too small");

    if (dz2 < 0)
        return new App::DocumentObjectExecReturn("delta z2 of wedge is negative");

    if (dx2 < 0)
        return new App::DocumentObjectExecReturn("delta x2 of wedge is negative");

    try {
        gp_Pnt pnt(0.0,0.0,0.0);
        gp_Dir dir(0.0,0.0,1.0);
        BRepPrim_Wedge mkWedge(gp_Ax2(pnt,dir),
            xmin, ymin, zmin, z2min, x2min,
            xmax, ymax, zmax, z2max, x2max);
        BRepBuilderAPI_MakeSolid mkSolid;
        mkSolid.Add(mkWedge.Shell());
        this->Shape.setValue(mkSolid.Solid());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

void Wedge::onChanged(const App::Property* prop)
{
    if (prop == &Xmin || prop == &Ymin || prop == &Zmin ||
        prop == &X2min || prop == &Z2min ||
        prop == &Xmax || prop == &Ymax || prop == &Zmax ||
        prop == &X2max || prop == &Z2max) {
        if (!isRestoring()) {
            App::DocumentObjectExecReturn *ret = recompute();
            delete ret;
        }
    }
    Part::Primitive::onChanged(prop);
}
