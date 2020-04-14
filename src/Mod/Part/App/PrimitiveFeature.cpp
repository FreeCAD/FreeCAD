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
# include <BRepPrimAPI_MakePrism.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepPrimAPI_MakeSphere.hxx>
# include <BRepPrimAPI_MakeTorus.hxx>
# include <BRepPrim_Wedge.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepBuilderAPI_MakeSolid.hxx>
# include <BRepBuilderAPI_MakePolygon.hxx>
# include <BRepBuilderAPI_GTransform.hxx>
# include <BRepProj_Projection.hxx>
# include <gp_Circ.hxx>
# include <gp_Elips.hxx>
# include <gp_GTrsf.hxx>
# include <GCE2d_MakeSegment.hxx>
# include <Geom_Plane.hxx>
# include <Geom_ConicalSurface.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom2d_Line.hxx>
# include <Geom2d_TrimmedCurve.hxx>
# include <Geom_Plane.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom2d_Line.hxx>
# include <Geom2d_TrimmedCurve.hxx>
# include <Precision.hxx>
# include <Standard_Real.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Vertex.hxx>
# include <Standard_Version.hxx>
#endif

#include "PrimitiveFeature.h"
#include <Mod/Part/App/PartFeaturePy.h>
#include <App/FeaturePythonPyImp.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Tools.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif


namespace Part {
    const App::PropertyQuantityConstraint::Constraints apexRange   = {0.0,90.0,0.1};
    const App::PropertyQuantityConstraint::Constraints torusRangeV = {-180.0,180.0,1.0};
    const App::PropertyQuantityConstraint::Constraints angleRangeU = {0.0,360.0,1.0};
    const App::PropertyQuantityConstraint::Constraints angleRangeV = {-90.0,90.0,1.0};
    const App::PropertyQuantityConstraint::Constraints quantityRange  = {0.0,FLT_MAX,0.1};
}

using namespace Part;


PROPERTY_SOURCE_ABSTRACT_WITH_EXTENSIONS(Part::Primitive, Part::Feature)

Primitive::Primitive(void)
{
    AttachExtension::initExtension(this);
    touch();
}

Primitive::~Primitive()
{
}

short Primitive::mustExecute(void) const
{
    return Feature::mustExecute();
}

App::DocumentObjectExecReturn* Primitive::execute(void) {
    return Part::Feature::execute();
}

namespace Part {
    PYTHON_TYPE_DEF(PrimitivePy, PartFeaturePy)
    PYTHON_TYPE_IMP(PrimitivePy, PartFeaturePy)
}

PyObject* Primitive::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new PrimitivePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

void Primitive::Restore(Base::XMLReader &reader)
{
    reader.readElement("Properties");
    int Cnt = reader.getAttributeAsInteger("Count");

    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Property");
        const char* PropName = reader.getAttribute("name");
        const char* TypeName = reader.getAttribute("type");
        App::Property* prop = getPropertyByName(PropName);
        // For #0001652 the property types of many primitive features have changed
        // from PropertyFloat or PropertyFloatConstraint to a more meaningful type.
        // In order to load older project files there must be checked in case the
        // types don't match if both inherit from PropertyFloat because all derived
        // classes do not re-implement the Save/Restore methods.
        try {
            if (prop && strcmp(prop->getTypeId().getName(), TypeName) == 0) {
                prop->Restore(reader);
            }
            else if (prop) {
                Base::Type inputType = Base::Type::fromName(TypeName);
                if (prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId()) &&
                    inputType.isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
                    // Do not directly call the property's Restore method in case the implementation
                    // has changed. So, create a temporary PropertyFloat object and assign the value.
                    App::PropertyFloat floatProp;
                    floatProp.Restore(reader);
                    static_cast<App::PropertyFloat*>(prop)->setValue(floatProp.getValue());
                }
            }
            else {
                extHandleChangedPropertyName(reader, TypeName, PropName); // AttachExtension
            }
        }
        catch (const Base::XMLParseException&) {
            throw; // re-throw
        }
        catch (const Base::Exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const std::exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const char* e) {
            Base::Console().Error("%s\n", e);
        }
#ifndef FC_DEBUG
        catch (...) {
            Base::Console().Error("Primitive::Restore: Unknown C++ exception thrown\n");
        }
#endif

        reader.readEndElement("Property");
    }
    reader.readEndElement("Properties");
}

void Primitive::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        // Do not support sphere, ellipsoid and torus because the creation
        // takes too long and thus is not feasible
        std::string grp = (prop->getGroup() ? prop->getGroup() : "");
        if (grp == "Plane" || grp == "Cylinder" || grp == "Cone") {
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
    return Part::Primitive::mustExecute();
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

    return Primitive::execute();
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
    Part::Primitive::onChanged(prop);
}

PROPERTY_SOURCE(Part::Line, Part::Primitive)

Line::Line()
{
    ADD_PROPERTY_TYPE(X1,(0.0),"Vertex 1 - Start",App::Prop_None,"X value of the start vertex");
    ADD_PROPERTY_TYPE(Y1,(0.0),"Vertex 1 - Start",App::Prop_None,"Y value of the Start vertex");
    ADD_PROPERTY_TYPE(Z1,(0.0),"Vertex 1 - Start",App::Prop_None,"Z value of the Start vertex");
    ADD_PROPERTY_TYPE(X2,(0.0),"Vertex 2 - Finish",App::Prop_None,"X value of the finish vertex");
    ADD_PROPERTY_TYPE(Y2,(0.0),"Vertex 2 - Finish",App::Prop_None,"Y value of the finish vertex");
    ADD_PROPERTY_TYPE(Z2,(1.0),"Vertex 2 - Finish",App::Prop_None,"Z value of the finish vertex");
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
    return Part::Primitive::mustExecute();
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

    return Primitive::execute();
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
    Part::Primitive::onChanged(prop);
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
    Handle(Geom_Plane) aPlane = new Geom_Plane(pnt, dir);
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
#if OCC_VERSION_HEX < 0x060500
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

    return Primitive::execute();
}

PROPERTY_SOURCE(Part::Sphere, Part::Primitive)

Sphere::Sphere(void)
{
    ADD_PROPERTY_TYPE(Radius,(5.0),"Sphere",App::Prop_None,"The radius of the sphere");
    Radius.setConstraints(&quantityRange);
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
                                        Angle1.getValue()/180.0f*M_PI,
                                        Angle2.getValue()/180.0f*M_PI,
                                        Angle3.getValue()/180.0f*M_PI);
        TopoDS_Shape ResultShape = mkSphere.Shape();
        this->Shape.setValue(ResultShape);
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return Primitive::execute();
}

PROPERTY_SOURCE(Part::Ellipsoid, Part::Primitive)

Ellipsoid::Ellipsoid(void)
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
}

short Ellipsoid::mustExecute() const
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
        TopoDS_Shape ResultShape = mkTrsf.Shape();
        this->Shape.setValue(ResultShape);
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return Primitive::execute();
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
                                        Angle.getValue()/180.0f*M_PI);
        TopoDS_Shape ResultShape = mkCylr.Shape();
        this->Shape.setValue(ResultShape);
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return Primitive::execute();
}

App::PropertyIntegerConstraint::Constraints Prism::polygonRange = {3,INT_MAX,1};

PROPERTY_SOURCE(Part::Prism, Part::Primitive)

Prism::Prism(void)
{
    ADD_PROPERTY_TYPE(Polygon,(6.0),"Prism",App::Prop_None,"Number of sides in the polygon, of the prism");
    ADD_PROPERTY_TYPE(Circumradius,(2.0),"Prism",App::Prop_None,"Circumradius (centre to vertex) of the polygon, of the prism");
    ADD_PROPERTY_TYPE(Height,(10.0f),"Prism",App::Prop_None,"The height of the prism");
    Polygon.setConstraints(&polygonRange);
}

short Prism::mustExecute() const
{
    if (Polygon.isTouched())
        return 1;
    if (Circumradius.isTouched())
        return 1;
    if (Height.isTouched())
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Prism::execute(void)
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
        this->Shape.setValue(mkPrism.Shape());
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return Primitive::execute();
}

App::PropertyIntegerConstraint::Constraints RegularPolygon::polygon = {3,INT_MAX,1};

PROPERTY_SOURCE(Part::RegularPolygon, Part::Primitive)

RegularPolygon::RegularPolygon(void)
{
    ADD_PROPERTY_TYPE(Polygon,(6.0),"RegularPolygon",App::Prop_None,"Number of sides in the regular polygon");
    ADD_PROPERTY_TYPE(Circumradius,(2.0),"RegularPolygon",App::Prop_None,"Circumradius (centre to vertex) of the polygon");
    Polygon.setConstraints(&polygon);
}

short RegularPolygon::mustExecute() const
{
    if (Polygon.isTouched())
        return 1;
    if (Circumradius.isTouched())
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *RegularPolygon::execute(void)
{
    // Build a regular polygon
    if (Polygon.getValue() < 3)
        return new App::DocumentObjectExecReturn("the polygon is invalid, must have 3 or more sides");
    if (Circumradius.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Circumradius of the polygon is too small");

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
        this->Shape.setValue(mkPoly.Shape());
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return Primitive::execute();
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
                                    Angle.getValue()/180.0f*M_PI);
        TopoDS_Shape ResultShape = mkCone.Shape();
        this->Shape.setValue(ResultShape);
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return Primitive::execute();
}

PROPERTY_SOURCE(Part::Torus, Part::Primitive)

Torus::Torus(void)
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
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return Primitive::execute();
}

PROPERTY_SOURCE(Part::Helix, Part::Primitive)

const char* Part::Helix::LocalCSEnums[]= {"Right-handed","Left-handed",NULL};
const char* Part::Helix::StyleEnums  []= {"Old style","New style",NULL};

Helix::Helix(void)
{
    ADD_PROPERTY_TYPE(Pitch, (1.0),"Helix",App::Prop_None,"The pitch of the helix");
    Pitch.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Height,(2.0),"Helix",App::Prop_None,"The height of the helix");
    Height.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Radius,(1.0),"Helix",App::Prop_None,"The radius of the helix");
    Radius.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Angle,(0.0),"Helix",App::Prop_None,"If angle is > 0 a conical otherwise a cylindircal surface is used");
    Angle.setConstraints(&apexRange);
    ADD_PROPERTY_TYPE(LocalCoord,(long(0)),"Coordinate System",App::Prop_None,"Orientation of the local coordinate system of the helix");
    LocalCoord.setEnums(LocalCSEnums);
    ADD_PROPERTY_TYPE(Style,(long(0)),"Helix style",App::Prop_Hidden,"Old style creates incorrect and new style create correct helices");
    Style.setEnums(StyleEnums);
}

void Helix::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Pitch || prop == &Height || prop == &Radius ||
            prop == &Angle || prop == &LocalCoord || prop == &Style) {
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    Part::Primitive::onChanged(prop);
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
    if (LocalCoord.isTouched())
        return 1;
    if (Style.isTouched())
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
        Standard_Boolean myLocalCS = LocalCoord.getValue() ? Standard_True : Standard_False;
        //Standard_Boolean myStyle = Style.getValue() ? Standard_True : Standard_False;
        TopoShape helix;
        // work around for OCC bug #23314 (FC #0954)
        // the exact conditions for failure are unknown.  building the helix 1 turn at a time
        // seems to always work.
        this->Shape.setValue(helix.makeLongHelix(myPitch, myHeight, myRadius, myAngle, myLocalCS));
//        if (myHeight / myPitch > 50.0)
//            this->Shape.setValue(helix.makeLongHelix(myPitch, myHeight, myRadius, myAngle, myLocalCS));
//        else
//            this->Shape.setValue(helix.makeHelix(myPitch, myHeight, myRadius, myAngle, myLocalCS, myStyle));
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return Primitive::execute();
}

PROPERTY_SOURCE(Part::Spiral, Part::Primitive)

Spiral::Spiral(void)
{
    ADD_PROPERTY_TYPE(Growth, (1.0),"Spiral",App::Prop_None,"The growth of the spiral per rotation");
    Growth.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Radius,(1.0),"Spiral",App::Prop_None,"The radius of the spiral");
    Radius.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Rotations,(2.0),"Spiral",App::Prop_None,"The number of rotations");
    Rotations.setConstraints(&quantityRange);
}

void Spiral::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Growth || prop == &Rotations || prop == &Radius) {
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    Part::Primitive::onChanged(prop);
}

short Spiral::mustExecute() const
{
    if (Growth.isTouched())
        return 1;
    if (Rotations.isTouched())
        return 1;
    if (Radius.isTouched())
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Spiral::execute(void)
{
    try {
        Standard_Real myNumRot = Rotations.getValue();
        Standard_Real myRadius = Radius.getValue();
        Standard_Real myGrowth = Growth.getValue();
        Standard_Real myPitch  = 1.0;
        Standard_Real myHeight = myNumRot * myPitch;
        Standard_Real myAngle  = atan(myGrowth / myPitch);
        TopoShape helix;

        if (myGrowth < Precision::Confusion())
            Standard_Failure::Raise("Growth too small");

        if (myNumRot < Precision::Confusion())
            Standard_Failure::Raise("Number of rotations too small");

        gp_Ax2 cylAx2(gp_Pnt(0.0,0.0,0.0) , gp::DZ());
        Handle(Geom_Surface) surf = new Geom_ConicalSurface(gp_Ax3(cylAx2), myAngle, myRadius);

        gp_Pnt2d aPnt(0, 0);
        gp_Dir2d aDir(2. * M_PI, myPitch);
        gp_Ax2d aAx2d(aPnt, aDir);

        Handle(Geom2d_Line) line = new Geom2d_Line(aAx2d);
        gp_Pnt2d beg = line->Value(0);
        gp_Pnt2d end = line->Value(sqrt(4.0*M_PI*M_PI+myPitch*myPitch)*(myHeight/myPitch));

        // calculate end point for conical helix
        Standard_Real v = myHeight / cos(myAngle);
        Standard_Real u = (myHeight/myPitch) * 2.0 * M_PI;
        gp_Pnt2d cend(u, v);
        end = cend;

        Handle(Geom2d_TrimmedCurve) segm = GCE2d_MakeSegment(beg , end);

        TopoDS_Edge edgeOnSurf = BRepBuilderAPI_MakeEdge(segm , surf);
        TopoDS_Wire wire = BRepBuilderAPI_MakeWire(edgeOnSurf);
        BRepLib::BuildCurves3d(wire);

        Handle(Geom_Plane) aPlane = new Geom_Plane(gp_Pnt(0.0,0.0,0.0), gp::DZ());
        Standard_Real range = (myNumRot+1) * myGrowth + 1 + myRadius;
        BRepBuilderAPI_MakeFace mkFace(aPlane, -range, range, -range, range
#if OCC_VERSION_HEX >= 0x060502
        , Precision::Confusion()
#endif
        );
        BRepProj_Projection proj(wire, mkFace.Face(), gp::DZ());
        this->Shape.setValue(proj.Shape());

        return Primitive::execute();
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
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
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return Primitive::execute();
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

App::PropertyQuantityConstraint::Constraints Ellipse::angleRange = {0.0,360.0,1.0};

PROPERTY_SOURCE(Part::Ellipse, Part::Primitive)


Ellipse::Ellipse()
{
    ADD_PROPERTY(MajorRadius,(4.0f));
    ADD_PROPERTY(MinorRadius,(4.0f));
    ADD_PROPERTY(Angle0,(0.0f));
    Angle0.setConstraints(&angleRange);
    ADD_PROPERTY(Angle1,(360.0f));
    Angle1.setConstraints(&angleRange);
}

Ellipse::~Ellipse()
{
}

short Ellipse::mustExecute() const
{
    if (Angle0.isTouched() ||
        Angle1.isTouched() ||
        MajorRadius.isTouched() ||
        MinorRadius.isTouched())
        return 1;
    return Part::Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Ellipse::execute(void)
{
    if (this->MinorRadius.getValue() > this->MajorRadius.getValue())
        return new App::DocumentObjectExecReturn("Minor radius greater than major radius");
    if (this->MinorRadius.getValue() < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Minor radius of ellipse too small");

    gp_Elips ellipse;
    ellipse.SetMajorRadius(this->MajorRadius.getValue());
    ellipse.SetMinorRadius(this->MinorRadius.getValue());

    BRepBuilderAPI_MakeEdge clMakeEdge(ellipse, Base::toRadians<double>(this->Angle0.getValue()),
                                                Base::toRadians<double>(this->Angle1.getValue()));
    const TopoDS_Edge& edge = clMakeEdge.Edge();
    this->Shape.setValue(edge);

    return Primitive::execute();
}

void Ellipse::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &MajorRadius || prop == &MinorRadius || prop == &Angle0 || prop == &Angle1){
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    Part::Primitive::onChanged(prop);
}
