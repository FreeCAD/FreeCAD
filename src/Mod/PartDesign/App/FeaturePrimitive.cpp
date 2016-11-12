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


#include "FeaturePrimitive.h"
#include "DatumPoint.h"
#include "DatumCS.h"
#include <Mod/Part/App/modelRefine.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <App/Document.h>
#include <App/Application.h>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <QObject>
#include <math.h>

using namespace PartDesign;

namespace PartDesign {

const App::PropertyQuantityConstraint::Constraints torusRangeV = {-180.0,180.0,1.0};
const App::PropertyQuantityConstraint::Constraints angleRangeU = {0.0,360.0,1.0};
const App::PropertyQuantityConstraint::Constraints angleRangeV = {-90.0,90.0,1.0};
const App::PropertyQuantityConstraint::Constraints quantityRange  = {0.0,FLT_MAX,0.1};

PROPERTY_SOURCE(PartDesign::FeaturePrimitive, PartDesign::FeatureAddSub)

FeaturePrimitive::FeaturePrimitive()
  :  primitiveType(Box)
{
    ADD_PROPERTY_TYPE(CoordinateSystem, (0), "Primitive", App::Prop_None, "References to build the location of the primitive");
}

TopoDS_Shape FeaturePrimitive::refineShapeIfActive(const TopoDS_Shape& oldShape) const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign");
    if (hGrp->GetBool("RefineModel", false)) {
        Part::BRepBuilderAPI_RefineModel mkRefine(oldShape);
        TopoDS_Shape resShape = mkRefine.Shape();
        return resShape;
    }

    return oldShape;
}

App::DocumentObjectExecReturn* FeaturePrimitive::execute(const TopoDS_Shape& primitiveShape)
{
    try {
        //transform the primitive in the correct coordinance
        App::DocumentObject* cs =  CoordinateSystem.getValue();
        if(cs && cs->getTypeId() == PartDesign::CoordinateSystem::getClassTypeId())
            Placement.setValue(static_cast<PartDesign::CoordinateSystem*>(cs)->Placement.getValue());
        else 
            Placement.setValue(Base::Placement());
        
        //if we have no base we just add the standard primitive shape
        TopoDS_Shape base;
        try{
             //if we have a base shape we need to make sure that it does not get our transformation to
             BRepBuilderAPI_Transform trsf(getBaseShape(), getLocation().Transformation().Inverted(), true);
             base = trsf.Shape();
        }
        catch(const Base::Exception&) {

             //as we use this for preview we can add it even if useless for subtractive
             AddSubShape.setValue(primitiveShape);
             
             if(getAddSubType() == FeatureAddSub::Additive)
                 Shape.setValue(getSolid(primitiveShape));
             else 
                 return new App::DocumentObjectExecReturn("Cannot subtract primitive feature without base feature");   
             
             return  App::DocumentObject::StdReturn;
        }
         
        if(getAddSubType() == FeatureAddSub::Additive) {
            
            BRepAlgoAPI_Fuse mkFuse(base, primitiveShape);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn("Adding the primitive failed");
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkFuse.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");
            
            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
            AddSubShape.setValue(primitiveShape);
        }
        else if(getAddSubType() == FeatureAddSub::Subtractive) {
            
            BRepAlgoAPI_Cut mkCut(base, primitiveShape);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn("Subtracting the primitive failed");
            // we have to get the solids (fuse sometimes creates compounds)
            TopoDS_Shape boolOp = this->getSolid(mkCut.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");
            
            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
            AddSubShape.setValue(primitiveShape);
        }
        
        
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

void FeaturePrimitive::onChanged(const App::Property* prop)
{
    FeatureAddSub::onChanged(prop);
}

PROPERTY_SOURCE(PartDesign::Box, PartDesign::FeaturePrimitive)

Box::Box()
{
    ADD_PROPERTY_TYPE(Length,(10.0f),"Box",App::Prop_None,"The length of the box");
    ADD_PROPERTY_TYPE(Width ,(10.0f),"Box",App::Prop_None,"The width of the box");
    ADD_PROPERTY_TYPE(Height,(10.0f),"Box",App::Prop_None,"The height of the box");
    Length.setConstraints(&quantityRange);
    Width.setConstraints(&quantityRange);
    Height.setConstraints(&quantityRange);
    
    primitiveType = FeaturePrimitive::Box;
}

App::DocumentObjectExecReturn* Box::execute(void)
{
    double L = Length.getValue();
    double W = Width.getValue();
    double H = Height.getValue();

    if (L < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Length of box too small");

    if (W < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Width of box too small");

    if (H < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Height of box too small");

    try {
        // Build a box using the dimension attributes
        BRepPrimAPI_MakeBox mkBox(L, W, H);
        return FeaturePrimitive::execute(mkBox.Shape());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

short int Box::mustExecute() const
{
     if ( Length.isTouched() ||
          Height.isTouched() ||
          Width.isTouched() )
        return 1;
     
    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveBox, PartDesign::Box)
PROPERTY_SOURCE(PartDesign::SubtractiveBox, PartDesign::Box)


PROPERTY_SOURCE(PartDesign::Cylinder, PartDesign::FeaturePrimitive)

Cylinder::Cylinder()
{
    ADD_PROPERTY_TYPE(Radius,(10.0f),"Cylinder",App::Prop_None,"The radius of the cylinder");
    ADD_PROPERTY_TYPE(Angle,(360.0f),"Cylinder",App::Prop_None,"The closing angle of the cylinder ");
    ADD_PROPERTY_TYPE(Height,(10.0f),"Cylinder",App::Prop_None,"The height of the cylinder");
    Angle.setConstraints(&angleRangeU);
    Radius.setConstraints(&quantityRange);
    Height.setConstraints(&quantityRange);
    
    primitiveType = FeaturePrimitive::Cylinder;
}

App::DocumentObjectExecReturn* Cylinder::execute(void)
{
    // Build a cylinder
    if (Radius.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Radius of cylinder too small");
    if (Height.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Height of cylinder too small");
    try {
        BRepPrimAPI_MakeCylinder mkCylr(Radius.getValue(),
                                        Height.getValue(),
                                        Angle.getValue()/180.0f*M_PI);
        
        return FeaturePrimitive::execute(mkCylr.Shape());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Cylinder::mustExecute() const
{
     if ( Radius.isTouched() ||
          Height.isTouched() ||
          Angle.isTouched() )
        return 1;
     
    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveCylinder, PartDesign::Cylinder)
PROPERTY_SOURCE(PartDesign::SubtractiveCylinder, PartDesign::Cylinder)


PROPERTY_SOURCE(PartDesign::Sphere, PartDesign::FeaturePrimitive)

Sphere::Sphere()
{
    ADD_PROPERTY_TYPE(Radius,(5.0),"Sphere",App::Prop_None,"The radius of the sphere");
    Radius.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Angle1,(-90.0f),"Sphere",App::Prop_None,"The angle of the sphere");
    Angle1.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle2,(90.0f),"Sphere",App::Prop_None,"The angle of the sphere");
    Angle2.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle3,(360.0f),"Sphere",App::Prop_None,"The angle of the sphere");
    Angle3.setConstraints(&angleRangeU);
    
    primitiveType = FeaturePrimitive::Sphere;
}

App::DocumentObjectExecReturn* Sphere::execute(void)
{
   // Build a sphere
    if (Radius.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Radius of sphere too small");
    try {
        BRepPrimAPI_MakeSphere mkSphere(Radius.getValue(),
                                        Angle1.getValue()/180.0f*M_PI,
                                        Angle2.getValue()/180.0f*M_PI,
                                        Angle3.getValue()/180.0f*M_PI);
        return FeaturePrimitive::execute(mkSphere.Shape());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Sphere::mustExecute() const
{
     if ( Radius.isTouched() ||
          Angle1.isTouched() ||
          Angle2.isTouched() ||
          Angle3.isTouched())
        return 1;
     
    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveSphere, PartDesign::Sphere)
PROPERTY_SOURCE(PartDesign::SubtractiveSphere, PartDesign::Sphere)


PROPERTY_SOURCE(PartDesign::Cone, PartDesign::FeaturePrimitive)

Cone::Cone()
{
    ADD_PROPERTY_TYPE(Radius1,(2.0),"Cone",App::Prop_None,"The radius of the cone");
    ADD_PROPERTY_TYPE(Radius2,(4.0),"Cone",App::Prop_None,"The radius of the cone");
    ADD_PROPERTY_TYPE(Height,(10.0),"Cone",App::Prop_None,"The height of the cone");
    ADD_PROPERTY_TYPE(Angle,(360.0),"Cone",App::Prop_None,"The angle of the cone");
    Angle.setConstraints(&angleRangeU);
    Radius1.setConstraints(&quantityRange);
    Radius2.setConstraints(&quantityRange);
    Height.setConstraints(&quantityRange);
    
    primitiveType = FeaturePrimitive::Cone;
}

App::DocumentObjectExecReturn* Cone::execute(void)
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
                                    Angle.getValue()/180.0f*M_PI);
        
        return FeaturePrimitive::execute(mkCone.Shape());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Cone::mustExecute() const
{
    if (Radius1.isTouched())
        return 1;
    if (Radius2.isTouched())
        return 1;
    if (Height.isTouched())
        return 1;
    if (Angle.isTouched())
        return 1;
    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveCone, PartDesign::Cone)
PROPERTY_SOURCE(PartDesign::SubtractiveCone, PartDesign::Cone)

PROPERTY_SOURCE(PartDesign::Ellipsoid, PartDesign::FeaturePrimitive)

Ellipsoid::Ellipsoid()
{
    ADD_PROPERTY_TYPE(Radius1,(2.0),"Ellipsoid",App::Prop_None,"The radius of the ellipsoid");
    Radius1.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Radius2,(4.0),"Ellipsoid",App::Prop_None,"The radius of the ellipsoid");
    Radius2.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Radius3,(0.0),"Ellipsoid",App::Prop_None,"The radius of the ellipsoid");
    Radius3.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Angle1,(-90.0f),"Ellipsoid",App::Prop_None,"The angle of the ellipsoid");
    Angle1.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle2,(90.0f),"Ellipsoid",App::Prop_None,"The angle of the ellipsoid");
    Angle2.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle3,(360.0f),"Ellipsoid",App::Prop_None,"The angle of the ellipsoid");
    Angle3.setConstraints(&angleRangeU);
    
    primitiveType = FeaturePrimitive::Ellipsoid;
}

App::DocumentObjectExecReturn* Ellipsoid::execute(void)
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
                                        Angle1.getValue()/180.0f*M_PI,
                                        Angle2.getValue()/180.0f*M_PI,
                                        Angle3.getValue()/180.0f*M_PI);
        Standard_Real scaleX = 1.0;
        Standard_Real scaleZ = Radius1.getValue()/Radius2.getValue();
        // issue #1798: A third radius has been introduced. To be backward
        // compatible if Radius3 is 0.0 (default) it's handled to be the same
        // as Radius2
        Standard_Real scaleY = 1.0;
        if (Radius3.getValue() >= Precision::Confusion())
            scaleY = Radius3.getValue()/Radius2.getValue();
        gp_GTrsf mat;
        mat.SetValue(1,1,scaleX);
        mat.SetValue(2,1,0.0);
        mat.SetValue(3,1,0.0);
        mat.SetValue(1,2,0.0);
        mat.SetValue(2,2,scaleY);
        mat.SetValue(3,2,0.0);
        mat.SetValue(1,3,0.0);
        mat.SetValue(2,3,0.0);
        mat.SetValue(3,3,scaleZ);
        BRepBuilderAPI_GTransform mkTrsf(mkSphere.Shape(), mat);
        return FeaturePrimitive::execute(mkTrsf.Shape());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Ellipsoid::mustExecute() const
{
    if (Radius1.isTouched())
        return 1;
    if (Radius2.isTouched())
        return 1;
    if (Radius3.isTouched())
        return 1;
    if (Angle1.isTouched())
        return 1;
    if (Angle2.isTouched())
        return 1;
    if (Angle3.isTouched())
        return 1;
    
    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveEllipsoid, PartDesign::Ellipsoid)
PROPERTY_SOURCE(PartDesign::SubtractiveEllipsoid, PartDesign::Ellipsoid)


PROPERTY_SOURCE(PartDesign::Torus, PartDesign::FeaturePrimitive)

Torus::Torus()
{
    ADD_PROPERTY_TYPE(Radius1,(10.0),"Torus",App::Prop_None,"The radius of the torus");
    Radius1.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Radius2,(2.0),"Torus",App::Prop_None,"The radius of the torus");
    Radius2.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Angle1,(-180.0),"Torus",App::Prop_None,"The angle of the torus");
    Angle1.setConstraints(&torusRangeV);
    ADD_PROPERTY_TYPE(Angle2,(180.0),"Torus",App::Prop_None,"The angle of the torus");
    Angle2.setConstraints(&torusRangeV);
    ADD_PROPERTY_TYPE(Angle3,(360.0),"Torus",App::Prop_None,"The angle of the torus");
    Angle3.setConstraints(&angleRangeU);
    
    primitiveType = FeaturePrimitive::Torus;
}

App::DocumentObjectExecReturn* Torus::execute(void)
{
    if (Radius1.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Radius of torus too small");
    if (Radius2.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Radius of torus too small");
    try {

        BRepPrimAPI_MakeTorus mkTorus(Radius1.getValue(),
                                      Radius2.getValue(),
                                      Angle1.getValue()/180.0f*M_PI,
                                      Angle2.getValue()/180.0f*M_PI,
                                      Angle3.getValue()/180.0f*M_PI);
        return FeaturePrimitive::execute(mkTorus.Solid());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Torus::mustExecute() const
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
    
    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveTorus, PartDesign::Torus)
PROPERTY_SOURCE(PartDesign::SubtractiveTorus, PartDesign::Torus)


PROPERTY_SOURCE(PartDesign::Prism, PartDesign::FeaturePrimitive)

Prism::Prism()
{
    ADD_PROPERTY_TYPE(Polygon,(6.0),"Prism",App::Prop_None,"Number of sides in the polygon, of the prism");
    ADD_PROPERTY_TYPE(Circumradius,(2.0),"Prism",App::Prop_None,"Circumradius (centre to vertex) of the polygon, of the prism");
    ADD_PROPERTY_TYPE(Height,(10.0f),"Prism",App::Prop_None,"The height of the prism");
    
    primitiveType = FeaturePrimitive::Prism;
}

App::DocumentObjectExecReturn* Prism::execute(void)
{
    // Build a prism
    if (Polygon.getValue() < 3)
        return new App::DocumentObjectExecReturn("Polygon of prism is invalid, must have 3 or more sides");
    if (Circumradius.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Circumradius of the polygon, of the prism, is too small");
    if (Height.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Height of prism is too small");
    try {
        long nodes = Polygon.getValue();

        Base::Matrix4D mat;
        mat.rotZ(Base::toRadians(360.0/nodes));

        // create polygon
        BRepBuilderAPI_MakePolygon mkPoly;
        Base::Vector3d v(Circumradius.getValue(),0,0);
        for (long i=0; i<nodes; i++) {
            mkPoly.Add(gp_Pnt(v.x,v.y,v.z));
            v = mat * v;
        }
        mkPoly.Add(gp_Pnt(v.x,v.y,v.z));
        BRepBuilderAPI_MakeFace mkFace(mkPoly.Wire());
        BRepPrimAPI_MakePrism mkPrism(mkFace.Face(), gp_Vec(0,0,Height.getValue()));
        return FeaturePrimitive::execute(mkPrism.Shape());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Prism::mustExecute() const
{
    if (Polygon.isTouched())
        return 1;
    if (Circumradius.isTouched())
        return 1;
    if (Height.isTouched())
        return 1;
    
    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditivePrism, PartDesign::Prism)
PROPERTY_SOURCE(PartDesign::SubtractivePrism, PartDesign::Prism)


PROPERTY_SOURCE(PartDesign::Wedge, PartDesign::FeaturePrimitive)

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
    
    primitiveType = FeaturePrimitive::Wedge;
}

App::DocumentObjectExecReturn* Wedge::execute(void)
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
        return FeaturePrimitive::execute(mkSolid.Solid());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Wedge::mustExecute() const
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
    
    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveWedge, PartDesign::Wedge)
PROPERTY_SOURCE(PartDesign::SubtractiveWedge, PartDesign::Wedge)
}
