/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <Geom_CartesianPoint.hxx>
# include <Geom_Circle.hxx>
# include <Geom_Curve.hxx>
# include <Geom_Ellipse.hxx>
# include <Geom_Hyperbola.hxx>
# include <Geom_Parabola.hxx>
# include <Geom_OffsetCurve.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <Geom_Line.hxx>
# include <Geom_BezierCurve.hxx>
# include <Geom_BezierSurface.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_Surface.hxx>
# include <Geom_Plane.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom_ConicalSurface.hxx>
# include <Geom_SphericalSurface.hxx>
# include <Geom_ToroidalSurface.hxx>
# include <Geom_OffsetSurface.hxx>
# include <Geom_RectangularTrimmedSurface.hxx>
# include <Geom_SurfaceOfRevolution.hxx>
# include <Geom_SurfaceOfLinearExtrusion.hxx>
# include <GeomConvert.hxx>
# include <GeomConvert_CompCurveToBSplineCurve.hxx>
# include <GeomLProp_CLProps.hxx>
# include <GeomLProp_SLProps.hxx>
# include <gp.hxx>
# include <gp_Circ.hxx>
# include <gp_Elips.hxx>
# include <gp_Hypr.hxx>
# include <gp_Parab.hxx>
# include <gce_ErrorType.hxx>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <gp_Pnt.hxx>
# include <gp_Cylinder.hxx>
# include <gp_Cone.hxx>
# include <gp_Sphere.hxx>
# include <gp_Torus.hxx>
# include <Standard_Real.hxx>
# include <Standard_Version.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TColgp_Array2OfPnt.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <gp.hxx>
# include <gp_Lin.hxx>
# include <Geom_Line.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <GC_MakeArcOfCircle.hxx>
# include <GC_MakeCircle.hxx>
# include <GC_MakeEllipse.hxx>
# include <GC_MakeLine.hxx>
# include <GC_MakeSegment.hxx>
# include <Precision.hxx>

#endif

#include "LinePy.h"
#include <Base/VectorPy.h>
#include "CirclePy.h"
#include "EllipsePy.h"
#include "ArcPy.h"
#include "ArcOfCirclePy.h"
#include "BezierCurvePy.h"
#include "BSplineCurvePy.h"
#include "HyperbolaPy.h"
#include "OffsetCurvePy.h"
#include "ParabolaPy.h"
#include "BezierSurfacePy.h"
#include "BSplineSurfacePy.h"
#include "ConePy.h"
#include "CylinderPy.h"
#include "OffsetSurfacePy.h"
#include "PlanePy.h"
#include "RectangularTrimmedSurfacePy.h"
#include "SpherePy.h"
#include "SurfaceOfExtrusionPy.h"
#include "SurfaceOfRevolutionPy.h"
#include "ToroidPy.h"

#include <Base/Exception.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Tools.h>

#include "Geometry.h"

using namespace Part;


const char* gce_ErrorStatusText(gce_ErrorType et)
{
    switch (et)
    {
    case gce_Done:
        return "Construction was successful";
    case gce_ConfusedPoints:
        return "Two points are coincident";
    case gce_NegativeRadius:
        return "Radius value is negative";
    case gce_ColinearPoints:
        return "Three points are collinear";
    case gce_IntersectionError:
        return "Intersection cannot be computed";
    case gce_NullAxis:
        return "Axis is undefined";
    case gce_NullAngle:
        return "Angle value is invalid (usually null)";
    case gce_NullRadius:
        return "Radius is null";
    case gce_InvertAxis:
        return "Axis value is invalid";
    case gce_BadAngle:
        return "Angle value is invalid";
    case gce_InvertRadius:
        return "Radius value is incorrect (usually with respect to another radius)";
    case gce_NullFocusLength:
        return "Focal distance is null";
    case gce_NullVector:
        return "Vector is null";
    case gce_BadEquation:
        return "Coefficients are incorrect (applies to the equation of a geometric object)";
    default:
        return "Creation of geometry failed";
    }
}

// ---------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Part::Geometry,Base::Persistence);

Geometry::Geometry()
  : Construction(false)
{
}

Geometry::~Geometry()
{
}

// Persistence implementer 
unsigned int Geometry::getMemSize (void) const               
{
    return 1;
}

void         Geometry::Save       (Base::Writer &writer) const 
{
    const char c = Construction?'1':'0';
    writer.Stream() << writer.ind() << "<Construction value=\"" <<  c << "\"/>" << endl;
}

void         Geometry::Restore    (Base::XMLReader &reader)    
{
    // read my Element
    reader.readElement("Construction");
    // get the value of my Attribute
    Construction = (int)reader.getAttributeAsInteger("value")==0?false:true;   
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomPoint,Part::Geometry);

GeomPoint::GeomPoint()
{
    this->myPoint = new Geom_CartesianPoint(0,0,0);
}

GeomPoint::GeomPoint(const Handle_Geom_CartesianPoint& p)
{
    this->myPoint = Handle_Geom_CartesianPoint::DownCast(p->Copy());
}

GeomPoint::GeomPoint(const Base::Vector3d& p)
{
    this->myPoint = new Geom_CartesianPoint(p.x,p.y,p.z);
}

GeomPoint::~GeomPoint()
{
}

const Handle_Geom_Geometry& GeomPoint::handle() const
{
    return myPoint;
}

Geometry *GeomPoint::clone(void) const
{
    GeomPoint *newPoint = new GeomPoint(myPoint);
    newPoint->Construction = this->Construction;
    return newPoint;
}

TopoDS_Shape GeomPoint::toShape() const
{
    return BRepBuilderAPI_MakeVertex(myPoint->Pnt());
}

Base::Vector3d GeomPoint::getPoint(void)const
{
    return Base::Vector3d(myPoint->X(),myPoint->Y(),myPoint->Z());
}

void GeomPoint::setPoint(const Base::Vector3d& p)
{
    this->myPoint->SetCoord(p.x,p.y,p.z);
}

// Persistence implementer 
unsigned int GeomPoint::getMemSize (void) const               
{
    return sizeof(Geom_CartesianPoint);
}

void GeomPoint::Save(Base::Writer &writer) const 
{
    // save the attributes of the father class
    Geometry::Save(writer);

    Base::Vector3d Point   =  getPoint();
    writer.Stream() 
         << writer.ind() 
             << "<GeomPoint " 
                << "X=\"" <<  Point.x << 
                "\" Y=\"" <<  Point.y << 
                "\" Z=\"" <<  Point.z << 
             "\"/>" << endl;
}

void GeomPoint::Restore(Base::XMLReader &reader) 
{
    // read the attributes of the father class
    Geometry::Restore(reader);

    double X,Y,Z;
    // read my Element
    reader.readElement("GeomPoint");
    // get the value of my Attribute
    X = reader.getAttributeAsFloat("X");
    Y = reader.getAttributeAsFloat("Y");
    Z = reader.getAttributeAsFloat("Z");
 
    // set the read geometry
    setPoint(Base::Vector3d(X,Y,Z) );
}

PyObject *GeomPoint::getPyObject(void)
{
    return new Base::VectorPy(getPoint());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Part::GeomCurve,Part::Geometry);

GeomCurve::GeomCurve()
{
}

GeomCurve::~GeomCurve()
{
}

TopoDS_Shape GeomCurve::toShape() const
{
    Handle_Geom_Curve c = Handle_Geom_Curve::DownCast(handle());
    BRepBuilderAPI_MakeEdge mkBuilder(c, c->FirstParameter(), c->LastParameter());
    return mkBuilder.Shape();
}

bool GeomCurve::tangent(double u, gp_Dir& dir) const
{
    Handle_Geom_Curve c = Handle_Geom_Curve::DownCast(handle());
    GeomLProp_CLProps prop(c,u,1,Precision::Confusion());
    if (prop.IsTangentDefined()) {
        prop.Tangent(dir);
        return true;
    }

    return false;
}


// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomBezierCurve,Part::GeomCurve);

GeomBezierCurve::GeomBezierCurve()
{
    TColgp_Array1OfPnt poles(1,2);
    poles(1) = gp_Pnt(0.0,0.0,0.0);
    poles(2) = gp_Pnt(0.0,0.0,1.0);
    Handle_Geom_BezierCurve b = new Geom_BezierCurve(poles);
    this->myCurve = b;
}

GeomBezierCurve::GeomBezierCurve(const Handle_Geom_BezierCurve& b)
{
    this->myCurve = Handle_Geom_BezierCurve::DownCast(b->Copy());
}

GeomBezierCurve::~GeomBezierCurve()
{
}

void GeomBezierCurve::setHandle(const Handle_Geom_BezierCurve& c)
{
    myCurve = Handle_Geom_BezierCurve::DownCast(c->Copy());
}

const Handle_Geom_Geometry& GeomBezierCurve::handle() const
{
    return myCurve;
}

Geometry *GeomBezierCurve::clone(void) const
{
    GeomBezierCurve *newCurve = new GeomBezierCurve(myCurve);
    newCurve->Construction = this->Construction;
    return newCurve;
}

// Persistence implementer 
unsigned int GeomBezierCurve::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomBezierCurve::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomBezierCurve::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomBezierCurve::getPyObject(void)
{
    return new BezierCurvePy((GeomBezierCurve*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomBSplineCurve,Part::GeomCurve);

GeomBSplineCurve::GeomBSplineCurve()
{
    TColgp_Array1OfPnt poles(1,2);
    poles(1) = gp_Pnt(0.0,0.0,0.0);
    poles(2) = gp_Pnt(1.0,0.0,0.0);

    TColStd_Array1OfReal knots(1,2);
    knots(1) = 0.0;
    knots(2) = 1.0;

    TColStd_Array1OfInteger mults(1,2);
    mults(1) = 2;
    mults(2) = 2;

    this->myCurve = new Geom_BSplineCurve(poles, knots, mults, 1);
}

GeomBSplineCurve::GeomBSplineCurve(const Handle_Geom_BSplineCurve& b)
{
    this->myCurve = Handle_Geom_BSplineCurve::DownCast(b->Copy());
}

GeomBSplineCurve::~GeomBSplineCurve()
{
}

void GeomBSplineCurve::setHandle(const Handle_Geom_BSplineCurve& c)
{
    myCurve = Handle_Geom_BSplineCurve::DownCast(c->Copy());
}

const Handle_Geom_Geometry& GeomBSplineCurve::handle() const
{
    return myCurve;
}

Geometry *GeomBSplineCurve::clone(void) const
{
    GeomBSplineCurve *newCurve = new GeomBSplineCurve(myCurve);
    newCurve->Construction = this->Construction;
    return newCurve;
}

int GeomBSplineCurve::countPoles() const
{
    return myCurve->NbPoles();
}

void GeomBSplineCurve::setPole(int index, const Base::Vector3d& pole, double weight)
{
    try {
        gp_Pnt pnt(pole.x,pole.y,pole.z);
        if (weight < 0.0)
            myCurve->SetPole(index+1,pnt);
        else
            myCurve->SetPole(index+1,pnt,weight);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        std::cout << e->GetMessageString() << std::endl;
    }
}

std::vector<Base::Vector3d> GeomBSplineCurve::getPoles() const
{
    std::vector<Base::Vector3d> poles;
    poles.reserve(myCurve->NbPoles());
    TColgp_Array1OfPnt p(1,myCurve->NbPoles());
    myCurve->Poles(p);

    for (Standard_Integer i=p.Lower(); i<=p.Upper(); i++) {
        const gp_Pnt& pnt = p(i);
        poles.push_back(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
    }
    return poles;
}

bool GeomBSplineCurve::join(const Handle_Geom_BSplineCurve& spline)
{
    GeomConvert_CompCurveToBSplineCurve ccbc(this->myCurve);
    if (!ccbc.Add(spline, Precision::Approximation()))
        return false;
    this->myCurve = ccbc.BSplineCurve();
    return true;
}

void GeomBSplineCurve::makeC1Continuous(double tol, double ang_tol)
{
    GeomConvert::C0BSplineToC1BSplineCurve(this->myCurve, tol, ang_tol);
}

// Persistence implementer 
unsigned int GeomBSplineCurve::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomBSplineCurve::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomBSplineCurve::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomBSplineCurve::getPyObject(void)
{
    return new BSplineCurvePy((GeomBSplineCurve*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomCircle,Part::GeomCurve);

GeomCircle::GeomCircle()
{
    Handle_Geom_Circle c = new Geom_Circle(gp_Circ());
    this->myCurve = c;
}

GeomCircle::GeomCircle(const Handle_Geom_Circle& c)
{
    this->myCurve = Handle_Geom_Circle::DownCast(c->Copy());
}

GeomCircle::~GeomCircle()
{
}

const Handle_Geom_Geometry& GeomCircle::handle() const
{
    return myCurve;
}

Geometry *GeomCircle::clone(void) const
{
    GeomCircle *newCirc = new GeomCircle(myCurve);
    newCirc->Construction = this->Construction;
    return newCirc;
}

Base::Vector3d GeomCircle::getCenter(void) const
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(handle());
    gp_Ax1 axis = circle->Axis();
    const gp_Pnt& loc = axis.Location();
    return Base::Vector3d(loc.X(),loc.Y(),loc.Z());
}

double GeomCircle::getRadius(void) const
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(handle());
    return circle->Radius();
}

void GeomCircle::setCenter(const Base::Vector3d& Center)
{
    gp_Pnt p1(Center.x,Center.y,Center.z);
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(handle());

    try {
        circle->SetLocation(p1);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

void GeomCircle::setRadius(double Radius)
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(handle());

    try {
        gp_Circ c = circle->Circ();
        c.SetRadius(Radius);
        circle->SetCirc(c);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

// Persistence implementer 
unsigned int GeomCircle::getMemSize (void) const
{
    return sizeof(Geom_Circle);
}

void GeomCircle::Save(Base::Writer& writer) const
{
    // save the attributes of the father class
    GeomCurve::Save(writer);

    gp_Pnt center = this->myCurve->Axis().Location();
    gp_Dir norm = this->myCurve->Axis().Direction();

    writer.Stream()
         << writer.ind()
             << "<Circle "
                << "CenterX=\"" <<  center.X() <<
                "\" CenterY=\"" <<  center.Y() <<
                "\" CenterZ=\"" <<  center.Z() <<
                "\" NormalX=\"" <<  norm.X() <<
                "\" NormalY=\"" <<  norm.Y() <<
                "\" NormalZ=\"" <<  norm.Z() <<
                "\" Radius=\"" <<  this->myCurve->Radius() <<
             "\"/>" << endl;
}

void GeomCircle::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    GeomCurve::Restore(reader);

    double CenterX,CenterY,CenterZ,NormalX,NormalY,NormalZ,Radius;
    // read my Element
    reader.readElement("Circle");
    // get the value of my Attribute
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    CenterZ = reader.getAttributeAsFloat("CenterZ");
    NormalX = reader.getAttributeAsFloat("NormalX");
    NormalY = reader.getAttributeAsFloat("NormalY");
    NormalZ = reader.getAttributeAsFloat("NormalZ");
    Radius = reader.getAttributeAsFloat("Radius");

    // set the read geometry
    gp_Pnt p1(CenterX,CenterY,CenterZ);
    gp_Dir norm(NormalX,NormalY,NormalZ);
    try {
        GC_MakeCircle mc(p1, norm, Radius);
        if (!mc.IsDone())
            throw Base::Exception(gce_ErrorStatusText(mc.Status()));

        this->myCurve = mc.Value();
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

PyObject *GeomCircle::getPyObject(void)
{
    return new CirclePy((GeomCircle*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomArcOfCircle,Part::GeomCurve);

GeomArcOfCircle::GeomArcOfCircle()
{
    Handle_Geom_Circle c = new Geom_Circle(gp_Circ());
    this->myCurve = new Geom_TrimmedCurve(c, c->FirstParameter(),c->LastParameter());
}

GeomArcOfCircle::GeomArcOfCircle(const Handle_Geom_Circle& c)
{
    this->myCurve = new Geom_TrimmedCurve(c, c->FirstParameter(),c->LastParameter());
}

GeomArcOfCircle::~GeomArcOfCircle()
{
}

void GeomArcOfCircle::setHandle(const Handle_Geom_TrimmedCurve& c)
{
    Handle_Geom_Circle basis = Handle_Geom_Circle::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not a circle");
    this->myCurve = Handle_Geom_TrimmedCurve::DownCast(c->Copy());
}

const Handle_Geom_Geometry& GeomArcOfCircle::handle() const
{
    return myCurve;
}

Geometry *GeomArcOfCircle::clone(void) const
{
    GeomArcOfCircle* copy = new GeomArcOfCircle();
    copy->setHandle(this->myCurve);
    copy->Construction = this->Construction;
    return copy;
}

Base::Vector3d GeomArcOfCircle::getStartPoint() const
{
    gp_Pnt pnt = this->myCurve->StartPoint();
    return Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
}

Base::Vector3d GeomArcOfCircle::getEndPoint() const
{
    gp_Pnt pnt = this->myCurve->EndPoint();
    return Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
}

Base::Vector3d GeomArcOfCircle::getCenter(void) const
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(myCurve->BasisCurve());
    gp_Ax1 axis = circle->Axis();
    const gp_Pnt& loc = axis.Location();
    return Base::Vector3d(loc.X(),loc.Y(),loc.Z());
}

double GeomArcOfCircle::getRadius(void) const
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(myCurve->BasisCurve());
    return circle->Radius();
}

void GeomArcOfCircle::setCenter(const Base::Vector3d& Center)
{
    gp_Pnt p1(Center.x,Center.y,Center.z);
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(myCurve->BasisCurve());

    try {
        circle->SetLocation(p1);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

void GeomArcOfCircle::setRadius(double Radius)
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(myCurve->BasisCurve());

    try {
        gp_Circ c = circle->Circ();
        c.SetRadius(Radius);
        circle->SetCirc(c);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

void GeomArcOfCircle::getRange(double& u, double& v) const
{
    u = myCurve->FirstParameter();
    v = myCurve->LastParameter();
}

void GeomArcOfCircle::setRange(double u, double v)
{
    try {
        myCurve->SetTrim(u, v);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

// Persistence implementer 
unsigned int GeomArcOfCircle::getMemSize (void) const
{
    return sizeof(Geom_Circle) + 2 *sizeof(double);
}

void GeomArcOfCircle::Save(Base::Writer &writer) const 
{
    // save the attributes of the father class
    Geometry::Save(writer);

    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(this->myCurve->BasisCurve());

    gp_Pnt center = circle->Axis().Location();
    gp_Dir norm = circle->Axis().Direction();

    writer.Stream()
         << writer.ind()
             << "<ArcOfCircle "
                << "CenterX=\"" <<  center.X() <<
                "\" CenterY=\"" <<  center.Y() <<
                "\" CenterZ=\"" <<  center.Z() <<
                "\" NormalX=\"" <<  norm.X() <<
                "\" NormalY=\"" <<  norm.Y() <<
                "\" NormalZ=\"" <<  norm.Z() <<
                "\" Radius=\"" <<  circle->Radius() <<
                "\" StartAngle=\"" <<  this->myCurve->FirstParameter() <<
                "\" EndAngle=\"" <<  this->myCurve->LastParameter() <<
             "\"/>" << endl;
}

void GeomArcOfCircle::Restore(Base::XMLReader &reader)    
{
    // read the attributes of the father class
    Geometry::Restore(reader);

    double CenterX,CenterY,CenterZ,NormalX,NormalY,NormalZ,Radius,StartAngle,EndAngle;
    // read my Element
    reader.readElement("ArcOfCircle");
    // get the value of my Attribute
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    CenterZ = reader.getAttributeAsFloat("CenterZ");
    NormalX = reader.getAttributeAsFloat("NormalX");
    NormalY = reader.getAttributeAsFloat("NormalY");
    NormalZ = reader.getAttributeAsFloat("NormalZ");
    Radius = reader.getAttributeAsFloat("Radius");
    StartAngle = reader.getAttributeAsFloat("StartAngle");
    EndAngle = reader.getAttributeAsFloat("EndAngle");

    // set the read geometry
    gp_Pnt p1(CenterX,CenterY,CenterZ);
    gp_Dir norm(NormalX,NormalY,NormalZ);
    try {
        GC_MakeCircle mc(p1, norm, Radius);
        if (!mc.IsDone())
            throw Base::Exception(gce_ErrorStatusText(mc.Status()));
        GC_MakeArcOfCircle ma(mc.Value()->Circ(), StartAngle, EndAngle, 1);
        if (!ma.IsDone())
            throw Base::Exception(gce_ErrorStatusText(ma.Status()));

        Handle_Geom_TrimmedCurve tmpcurve = ma.Value();
        Handle_Geom_Circle tmpcircle = Handle_Geom_Circle::DownCast(tmpcurve->BasisCurve());
        Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(this->myCurve->BasisCurve());
 
        circle->SetCirc(tmpcircle->Circ());
        this->myCurve->SetTrim(tmpcurve->FirstParameter(), tmpcurve->LastParameter());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

PyObject *GeomArcOfCircle::getPyObject(void)
{
    return new ArcOfCirclePy(static_cast<GeomArcOfCircle*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomEllipse,Part::GeomCurve);

GeomEllipse::GeomEllipse()
{
    Handle_Geom_Ellipse e = new Geom_Ellipse(gp_Elips());
    this->myCurve = e;
}

GeomEllipse::GeomEllipse(const Handle_Geom_Ellipse& e)
{
    this->myCurve = Handle_Geom_Ellipse::DownCast(e->Copy());
}

GeomEllipse::~GeomEllipse()
{
}

const Handle_Geom_Geometry& GeomEllipse::handle() const
{
    return myCurve;
}

Geometry *GeomEllipse::clone(void) const
{
    GeomEllipse *newEllipse = new GeomEllipse(myCurve);
    newEllipse->Construction = this->Construction;
    return newEllipse;
}

Base::Vector3d GeomEllipse::getCenter(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(handle());
    gp_Ax1 axis = ellipse->Axis();
    const gp_Pnt& loc = axis.Location();
    return Base::Vector3d(loc.X(),loc.Y(),loc.Z());
}

void GeomEllipse::setCenter(const Base::Vector3d& Center)
{
    gp_Pnt p1(Center.x,Center.y,Center.z);
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(handle());

    try {
        ellipse->SetLocation(p1);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

double GeomEllipse::getMajorRadius(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(handle());
    return ellipse->MajorRadius();
}

void GeomEllipse::setMajorRadius(double Radius)
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(handle());

    try {
        ellipse->SetMajorRadius(Radius);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

double GeomEllipse::getMinorRadius(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(handle());
    return ellipse->MinorRadius();
}

void GeomEllipse::setMinorRadius(double Radius)
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(handle());

    try {
        ellipse->SetMinorRadius(Radius);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

// Persistence implementer 
unsigned int GeomEllipse::getMemSize (void) const
{
    return sizeof(Geom_Ellipse);
}

void GeomEllipse::Save(Base::Writer& writer) const
{
    // save the attributes of the father class
    GeomCurve::Save(writer);

    gp_Pnt center = this->myCurve->Axis().Location();
    gp_Dir normal = this->myCurve->Axis().Direction();

    writer.Stream()
         << writer.ind()
            << "<Ellipse "
            << "CenterX=\"" <<  center.X() << "\" "
            << "CenterY=\"" <<  center.Y() << "\" "
            << "CenterZ=\"" <<  center.Z() << "\" "
            << "NormalX=\"" <<  normal.X() << "\" "
            << "NormalY=\"" <<  normal.Y() << "\" "
            << "NormalZ=\"" <<  normal.Z() << "\" "
            << "MajorRadius=\"" <<  this->myCurve->MajorRadius() << "\" "
            << "MinorRadius=\"" <<  this->myCurve->MinorRadius() << "\" "
            << "/>" << endl;
}

void GeomEllipse::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    GeomCurve::Restore(reader);

    double CenterX,CenterY,CenterZ,NormalX,NormalY,NormalZ,MajorRadius,MinorRadius;
    // read my Element
    reader.readElement("Ellipse");
    // get the value of my Attribute
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    CenterZ = reader.getAttributeAsFloat("CenterZ");
    NormalX = reader.getAttributeAsFloat("NormalX");
    NormalY = reader.getAttributeAsFloat("NormalY");
    NormalZ = reader.getAttributeAsFloat("NormalZ");
    MajorRadius = reader.getAttributeAsFloat("MajorRadius");
    MinorRadius = reader.getAttributeAsFloat("MinorRadius");

    // set the read geometry
    gp_Pnt p1(CenterX,CenterY,CenterZ);
    gp_Dir norm(NormalX,NormalY,NormalZ);
    try {
        GC_MakeEllipse mc(gp_Ax2(p1, norm), MajorRadius, MinorRadius);
        if (!mc.IsDone())
            throw Base::Exception(gce_ErrorStatusText(mc.Status()));

        this->myCurve = mc.Value();
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

PyObject *GeomEllipse::getPyObject(void)
{
    return new EllipsePy((GeomEllipse*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomHyperbola,Part::GeomCurve);

GeomHyperbola::GeomHyperbola()
{
    Handle_Geom_Hyperbola h = new Geom_Hyperbola(gp_Hypr());
    this->myCurve = h;
}

GeomHyperbola::GeomHyperbola(const Handle_Geom_Hyperbola& h)
{
    this->myCurve = Handle_Geom_Hyperbola::DownCast(h->Copy());
}

GeomHyperbola::~GeomHyperbola()
{
}

const Handle_Geom_Geometry& GeomHyperbola::handle() const
{
    return myCurve;
}

Geometry *GeomHyperbola::clone(void) const
{
    GeomHyperbola *newHyp = new GeomHyperbola(myCurve);
    newHyp->Construction = this->Construction;
    return newHyp;
}

// Persistence implementer 
unsigned int GeomHyperbola::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomHyperbola::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomHyperbola::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomHyperbola::getPyObject(void)
{
    return new HyperbolaPy((GeomHyperbola*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomParabola,Part::GeomCurve);

GeomParabola::GeomParabola()
{
    Handle_Geom_Parabola p = new Geom_Parabola(gp_Parab());
    this->myCurve = p;
}

GeomParabola::GeomParabola(const Handle_Geom_Parabola& p)
{
    this->myCurve = Handle_Geom_Parabola::DownCast(p->Copy());
}

GeomParabola::~GeomParabola()
{
}

const Handle_Geom_Geometry& GeomParabola::handle() const
{
    return myCurve;
}

Geometry *GeomParabola::clone(void) const
{
    GeomParabola *newPar = new GeomParabola(myCurve);
    newPar->Construction = this->Construction;
    return newPar;
}

// Persistence implementer 
unsigned int GeomParabola::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomParabola::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomParabola::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomParabola::getPyObject(void)
{
    return new ParabolaPy((GeomParabola*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomLine,Part::GeomCurve);

GeomLine::GeomLine()
{
    Handle_Geom_Line c = new Geom_Line(gp_Lin());
    this->myCurve = c;
}

GeomLine::GeomLine(const Handle_Geom_Line& l)
{
    this->myCurve = Handle_Geom_Line::DownCast(l->Copy());
}

GeomLine::GeomLine(const Base::Vector3d& Pos, const Base::Vector3d& Dir)
{
    this->myCurve = new Geom_Line(gp_Pnt(Pos.x,Pos.y,Pos.z),gp_Dir(Dir.x,Dir.y,Dir.z));
}


GeomLine::~GeomLine()
{
}

void GeomLine::setLine(const Base::Vector3d& Pos, const Base::Vector3d& Dir)
{
    this->myCurve->SetLocation(gp_Pnt(Pos.x,Pos.y,Pos.z));
    this->myCurve->SetDirection(gp_Dir(Dir.x,Dir.y,Dir.z));
}

Base::Vector3d GeomLine::getPos(void) const
{
    gp_Pnt Pos = this->myCurve->Lin().Location();
    return Base::Vector3d(Pos.X(),Pos.Y(),Pos.Z());
}

Base::Vector3d GeomLine::getDir(void) const
{
    gp_Dir Dir = this->myCurve->Lin().Direction();
    return Base::Vector3d(Dir.X(),Dir.Y(),Dir.Z());
}

const Handle_Geom_Geometry& GeomLine::handle() const
{
    return myCurve;
}

Geometry *GeomLine::clone(void) const
{
    GeomLine *newLine = new GeomLine(myCurve);
    newLine->Construction = this->Construction;
    return newLine;
}

// Persistence implementer 
unsigned int GeomLine::getMemSize (void) const 
{
    return sizeof(Geom_Line);
}

void GeomLine::Save(Base::Writer &writer) const 
{
    // save the attributes of the father class
    Geometry::Save(writer);

    Base::Vector3d Pos   =  getPos();
    Base::Vector3d Dir   =  getDir();

    writer.Stream() 
         << writer.ind() 
             << "<GeomLine " 
                << "PosX=\"" <<  Pos.x << 
                "\" PosY=\"" <<  Pos.y << 
                "\" PosZ=\"" <<  Pos.z << 
                "\" DirX=\"" <<  Dir.x << 
                "\" DirY=\"" <<  Dir.y << 
                "\" DirZ=\"" <<  Dir.z <<
             "\"/>" << endl;
}
void GeomLine::Restore(Base::XMLReader &reader)
{
    // read the attributes of the father class
    Geometry::Restore(reader);

    double PosX,PosY,PosZ,DirX,DirY,DirZ;
    // read my Element
    reader.readElement("GeomLine");
    // get the value of my Attribute
    PosX = reader.getAttributeAsFloat("PosX");
    PosY = reader.getAttributeAsFloat("PosY");
    PosZ = reader.getAttributeAsFloat("PosZ");
    DirX = reader.getAttributeAsFloat("DirX");
    DirY = reader.getAttributeAsFloat("DirY");
    DirZ = reader.getAttributeAsFloat("DirZ");
 
    // set the read geometry
    setLine(Base::Vector3d(PosX,PosY,PosZ),Base::Vector3d(DirX,DirY,DirZ) );
}

PyObject *GeomLine::getPyObject(void)
{
    return 0;
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomLineSegment,Part::GeomCurve);

GeomLineSegment::GeomLineSegment()
{
    gp_Lin line;
    line.SetLocation(gp_Pnt(0.0,0.0,0.0));
    line.SetDirection(gp_Dir(0.0,0.0,1.0));
    Handle_Geom_Line c = new Geom_Line(line);
    this->myCurve = new Geom_TrimmedCurve(c, 0.0,1.0);
}

GeomLineSegment::~GeomLineSegment()
{
}

void GeomLineSegment::setHandle(const Handle_Geom_TrimmedCurve& c)
{
    Handle_Geom_Line basis = Handle_Geom_Line::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not a line");
    this->myCurve = Handle_Geom_TrimmedCurve::DownCast(c->Copy());
}

const Handle_Geom_Geometry& GeomLineSegment::handle() const
{
    return myCurve;
}

Geometry *GeomLineSegment::clone(void)const
{
    GeomLineSegment *tempCurve = new GeomLineSegment();
    tempCurve->myCurve = Handle_Geom_TrimmedCurve::DownCast(myCurve->Copy());
    tempCurve->Construction = this->Construction;
    return tempCurve;
}

Base::Vector3d GeomLineSegment::getStartPoint() const
{
    Handle_Geom_TrimmedCurve this_curve = Handle_Geom_TrimmedCurve::DownCast(handle());
    gp_Pnt pnt = this_curve->StartPoint();
    return Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
}

Base::Vector3d GeomLineSegment::getEndPoint() const
{
    Handle_Geom_TrimmedCurve this_curve = Handle_Geom_TrimmedCurve::DownCast(handle());
    gp_Pnt pnt = this_curve->EndPoint();
    return Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
}

void GeomLineSegment::setPoints(const Base::Vector3d& Start, const Base::Vector3d& End)
{
    gp_Pnt p1(Start.x,Start.y,Start.z), p2(End.x,End.y,End.z);
    Handle_Geom_TrimmedCurve this_curv = Handle_Geom_TrimmedCurve::DownCast(handle());

    try {
        // Create line out of two points
        if (p1.Distance(p2) < gp::Resolution())
            Standard_Failure::Raise("Both points are equal");
        GC_MakeSegment ms(p1, p2);
        if (!ms.IsDone()) {
            throw Base::Exception(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line segment
        Handle_Geom_Line this_line = Handle_Geom_Line::DownCast
            (this_curv->BasisCurve());
        Handle_Geom_TrimmedCurve that_curv = ms.Value();
        Handle_Geom_Line that_line = Handle_Geom_Line::DownCast(that_curv->BasisCurve());
        this_line->SetLin(that_line->Lin());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Base::Exception(e->GetMessageString());
    }
}

// Persistence implementer 
unsigned int GeomLineSegment::getMemSize (void) const
{
    return sizeof(Geom_TrimmedCurve) + sizeof(Geom_Line);
}

void GeomLineSegment::Save       (Base::Writer &writer) const 
{
    // save the attributes of the father class
    Geometry::Save(writer);

    Base::Vector3d End   =  getEndPoint();
    Base::Vector3d Start =  getStartPoint();

    writer.Stream() 
         << writer.ind() 
             << "<LineSegment " 
                << "StartX=\"" <<  Start.x << 
                "\" StartY=\"" <<  Start.y << 
                "\" StartZ=\"" <<  Start.z << 
                "\" EndX=\"" <<  End.x << 
                "\" EndY=\"" <<  End.y << 
                "\" EndZ=\"" <<  End.z <<
             "\"/>" << endl;
}

void GeomLineSegment::Restore    (Base::XMLReader &reader)    
{
    // read the attributes of the father class
    Geometry::Restore(reader);

    double StartX,StartY,StartZ,EndX,EndY,EndZ;
    // read my Element
    reader.readElement("LineSegment");
    // get the value of my Attribute
    StartX = reader.getAttributeAsFloat("StartX");
    StartY = reader.getAttributeAsFloat("StartY");
    StartZ = reader.getAttributeAsFloat("StartZ");
    EndX   = reader.getAttributeAsFloat("EndX");
    EndY   = reader.getAttributeAsFloat("EndY");
    EndZ   = reader.getAttributeAsFloat("EndZ");
 
    // set the read geometry
    setPoints(Base::Vector3d(StartX,StartY,StartZ),Base::Vector3d(EndX,EndY,EndZ) );
}

PyObject *GeomLineSegment::getPyObject(void)
{
    return new LinePy((GeomLineSegment*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomOffsetCurve,Part::GeomCurve);

GeomOffsetCurve::GeomOffsetCurve()
{
}

GeomOffsetCurve::GeomOffsetCurve(const Handle_Geom_Curve& c, double offset, const gp_Dir& dir)
{
    this->myCurve = new Geom_OffsetCurve(c, offset, dir);
}

GeomOffsetCurve::GeomOffsetCurve(const Handle_Geom_OffsetCurve& c)
{
    this->myCurve = Handle_Geom_OffsetCurve::DownCast(c->Copy());
}

GeomOffsetCurve::~GeomOffsetCurve()
{
}

Geometry *GeomOffsetCurve::clone(void) const
{
    GeomOffsetCurve *newCurve = new GeomOffsetCurve(myCurve);
    newCurve->Construction = this->Construction;
    return newCurve;
}

void GeomOffsetCurve::setHandle(const Handle_Geom_OffsetCurve& c)
{
    this->myCurve = Handle_Geom_OffsetCurve::DownCast(c->Copy());
}

const Handle_Geom_Geometry& GeomOffsetCurve::handle() const
{
    return this->myCurve;
}

// Persistence implementer 
unsigned int GeomOffsetCurve::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomOffsetCurve::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomOffsetCurve::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomOffsetCurve::getPyObject(void)
{
    return new OffsetCurvePy((GeomOffsetCurve*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomTrimmedCurve,Part::GeomCurve);

GeomTrimmedCurve::GeomTrimmedCurve()
{
}

GeomTrimmedCurve::GeomTrimmedCurve(const Handle_Geom_TrimmedCurve& c)
{
    this->myCurve = Handle_Geom_TrimmedCurve::DownCast(c->Copy());
}

GeomTrimmedCurve::~GeomTrimmedCurve()
{
}

void GeomTrimmedCurve::setHandle(const Handle_Geom_TrimmedCurve& c)
{
    this->myCurve = Handle_Geom_TrimmedCurve::DownCast(c->Copy());
}

const Handle_Geom_Geometry& GeomTrimmedCurve::handle() const
{
    return myCurve;
}

Geometry *GeomTrimmedCurve::clone(void) const
{
    GeomTrimmedCurve *newCurve =  new GeomTrimmedCurve(myCurve);
    newCurve->Construction = this->Construction;
    return newCurve;
}

// Persistence implementer 
unsigned int GeomTrimmedCurve::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomTrimmedCurve::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomTrimmedCurve::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomTrimmedCurve::getPyObject(void)
{
    return 0;
}

// -------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Part::GeomSurface,Part::Geometry);

GeomSurface::GeomSurface()
{
}

GeomSurface::~GeomSurface()
{
}

TopoDS_Shape GeomSurface::toShape() const
{
    Handle_Geom_Surface s = Handle_Geom_Surface::DownCast(handle());
    Standard_Real u1,u2,v1,v2;
    s->Bounds(u1,u2,v1,v2);
    BRepBuilderAPI_MakeFace mkBuilder(s, u1, u2, v1, v2
#if OCC_VERSION_HEX >= 0x060502
      , Precision::Confusion()
#endif
      );
    return mkBuilder.Shape();
}

bool GeomSurface::tangentU(double u, double v, gp_Dir& dirU) const
{
    Handle_Geom_Surface s = Handle_Geom_Surface::DownCast(handle());
    GeomLProp_SLProps prop(s,u,v,1,Precision::Confusion());
    if (prop.IsTangentUDefined()) {
        prop.TangentU(dirU);
        return true;
    }

    return false;
}

bool GeomSurface::tangentV(double u, double v, gp_Dir& dirV) const
{
    Handle_Geom_Surface s = Handle_Geom_Surface::DownCast(handle());
    GeomLProp_SLProps prop(s,u,v,1,Precision::Confusion());
    if (prop.IsTangentVDefined()) {
        prop.TangentV(dirV);
        return true;
    }

    return false;
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomBezierSurface,Part::GeomSurface);

GeomBezierSurface::GeomBezierSurface()
{
    TColgp_Array2OfPnt poles(1,2,1,2);
    poles(1,1) = gp_Pnt(0.0,0.0,0.0);
    poles(2,1) = gp_Pnt(1.0,0.0,0.0);
    poles(1,2) = gp_Pnt(0.0,1.0,0.0);
    poles(2,2) = gp_Pnt(1.0,1.0,0.0);
    this->mySurface = new Geom_BezierSurface(poles);
}

GeomBezierSurface::GeomBezierSurface(const Handle_Geom_BezierSurface& b)
{
    this->mySurface = Handle_Geom_BezierSurface::DownCast(b->Copy());
}

GeomBezierSurface::~GeomBezierSurface()
{
}

const Handle_Geom_Geometry& GeomBezierSurface::handle() const
{
    return mySurface;
}

Geometry *GeomBezierSurface::clone(void) const
{
    GeomBezierSurface *newSurf =  new GeomBezierSurface(mySurface);
    newSurf->Construction = this->Construction;
    return newSurf;
}

// Persistence implementer 
unsigned int GeomBezierSurface::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomBezierSurface::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomBezierSurface::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomBezierSurface::getPyObject(void)
{
    return new BezierSurfacePy((GeomBezierSurface*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomBSplineSurface,Part::GeomSurface);

GeomBSplineSurface::GeomBSplineSurface()
{
    TColgp_Array2OfPnt poles(1,2,1,2);
    poles(1,1) = gp_Pnt(0.0,0.0,0.0);
    poles(2,1) = gp_Pnt(1.0,0.0,0.0);
    poles(1,2) = gp_Pnt(0.0,1.0,0.0);
    poles(2,2) = gp_Pnt(1.0,1.0,0.0);

    TColStd_Array1OfReal knots(1,2);
    knots(1) = 0.0;
    knots(2) = 1.0;

    TColStd_Array1OfInteger mults(1,2);
    mults(1) = 2;
    mults(2) = 2;

    this->mySurface = new Geom_BSplineSurface(poles, knots, knots, mults, mults, 1, 1);
}

GeomBSplineSurface::GeomBSplineSurface(const Handle_Geom_BSplineSurface& b)
{
    this->mySurface = Handle_Geom_BSplineSurface::DownCast(b->Copy());
}

GeomBSplineSurface::~GeomBSplineSurface()
{
}

void GeomBSplineSurface::setHandle(const Handle_Geom_BSplineSurface& s)
{
    mySurface = Handle_Geom_BSplineSurface::DownCast(s->Copy());
}

const Handle_Geom_Geometry& GeomBSplineSurface::handle() const
{
    return mySurface;
}

Geometry *GeomBSplineSurface::clone(void) const
{
    GeomBSplineSurface *newSurf =  new GeomBSplineSurface(mySurface);
    newSurf->Construction = this->Construction;
    return newSurf;
}

// Persistence implementer 
unsigned int GeomBSplineSurface::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomBSplineSurface::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomBSplineSurface::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomBSplineSurface::getPyObject(void)
{
    return new BSplineSurfacePy((GeomBSplineSurface*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomCylinder,Part::GeomSurface);

GeomCylinder::GeomCylinder()
{
    Handle_Geom_CylindricalSurface s = new Geom_CylindricalSurface(gp_Cylinder());
    this->mySurface = s;
}

GeomCylinder::~GeomCylinder()
{
}

const Handle_Geom_Geometry& GeomCylinder::handle() const
{
    return mySurface;
}

Geometry *GeomCylinder::clone(void) const
{
    GeomCylinder *tempCurve = new GeomCylinder();
    tempCurve->mySurface = Handle_Geom_CylindricalSurface::DownCast(mySurface->Copy());
    tempCurve->Construction = this->Construction;
    return tempCurve;
}

// Persistence implementer 
unsigned int GeomCylinder::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomCylinder::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomCylinder::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomCylinder::getPyObject(void)
{
    return new CylinderPy((GeomCylinder*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomCone,Part::GeomSurface);

GeomCone::GeomCone()
{
    Handle_Geom_ConicalSurface s = new Geom_ConicalSurface(gp_Cone());
    this->mySurface = s;
}

GeomCone::~GeomCone()
{
}

const Handle_Geom_Geometry& GeomCone::handle() const
{
    return mySurface;
}

Geometry *GeomCone::clone(void) const
{
    GeomCone *tempCurve = new GeomCone();
    tempCurve->mySurface = Handle_Geom_ConicalSurface::DownCast(mySurface->Copy());
    tempCurve->Construction = this->Construction;
    return tempCurve;
}

// Persistence implementer 
unsigned int GeomCone::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomCone::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomCone::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomCone::getPyObject(void)
{
    return new ConePy((GeomCone*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomToroid,Part::GeomSurface);

GeomToroid::GeomToroid()
{
    Handle_Geom_ToroidalSurface s = new Geom_ToroidalSurface(gp_Torus());
    this->mySurface = s;
}

GeomToroid::~GeomToroid()
{
}

const Handle_Geom_Geometry& GeomToroid::handle() const
{
    return mySurface;
}

Geometry *GeomToroid::clone(void) const
{
    GeomToroid *tempCurve = new GeomToroid();
    tempCurve->mySurface = Handle_Geom_ToroidalSurface::DownCast(mySurface->Copy());
    tempCurve->Construction = this->Construction;
    return tempCurve;
}

// Persistence implementer 
unsigned int GeomToroid::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomToroid::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomToroid::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomToroid::getPyObject(void)
{
    return new ToroidPy((GeomToroid*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomSphere,Part::GeomSurface);

GeomSphere::GeomSphere()
{
    Handle_Geom_SphericalSurface s = new Geom_SphericalSurface(gp_Sphere());
    this->mySurface = s;
}

GeomSphere::~GeomSphere()
{
}

const Handle_Geom_Geometry& GeomSphere::handle() const
{
    return mySurface;
}

Geometry *GeomSphere::clone(void) const
{
    GeomSphere *tempCurve = new GeomSphere();
    tempCurve->mySurface = Handle_Geom_SphericalSurface::DownCast(mySurface->Copy());
    tempCurve->Construction = this->Construction;
    return tempCurve;
}

// Persistence implementer 
unsigned int GeomSphere::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomSphere::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomSphere::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomSphere::getPyObject(void)
{
    return new SpherePy((GeomSphere*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomPlane,Part::GeomSurface);

GeomPlane::GeomPlane()
{
    Handle_Geom_Plane s = new Geom_Plane(gp_Pln());
    this->mySurface = s;
}

GeomPlane::~GeomPlane()
{
}

const Handle_Geom_Geometry& GeomPlane::handle() const
{
    return mySurface;
}

Geometry *GeomPlane::clone(void) const
{
    GeomPlane *tempCurve = new GeomPlane();
    tempCurve->mySurface = Handle_Geom_Plane::DownCast(mySurface->Copy());
    tempCurve->Construction = this->Construction;
    return tempCurve;
}

// Persistence implementer 
unsigned int GeomPlane::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomPlane::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomPlane::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomPlane::getPyObject(void)
{
    return new PlanePy((GeomPlane*)this->clone());
}
// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomOffsetSurface,Part::GeomSurface);

GeomOffsetSurface::GeomOffsetSurface()
{
}

GeomOffsetSurface::GeomOffsetSurface(const Handle_Geom_Surface& s, double offset)
{
    this->mySurface = new Geom_OffsetSurface(s, offset);
}

GeomOffsetSurface::GeomOffsetSurface(const Handle_Geom_OffsetSurface& s)
{
    this->mySurface = Handle_Geom_OffsetSurface::DownCast(s->Copy());
}

GeomOffsetSurface::~GeomOffsetSurface()
{
}

void GeomOffsetSurface::setHandle(const Handle_Geom_OffsetSurface& s)
{
    mySurface = Handle_Geom_OffsetSurface::DownCast(s->Copy());
}

const Handle_Geom_Geometry& GeomOffsetSurface::handle() const
{
    return mySurface;
}

Geometry *GeomOffsetSurface::clone(void) const
{
    GeomOffsetSurface *newSurf = new GeomOffsetSurface(mySurface);
    newSurf->Construction = this->Construction;
    return newSurf;
}

// Persistence implementer 
unsigned int GeomOffsetSurface::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomOffsetSurface::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomOffsetSurface::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomOffsetSurface::getPyObject(void)
{
    return new OffsetSurfacePy((GeomOffsetSurface*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomTrimmedSurface,Part::GeomSurface);

GeomTrimmedSurface::GeomTrimmedSurface()
{
}

GeomTrimmedSurface::GeomTrimmedSurface(const Handle_Geom_RectangularTrimmedSurface& s)
{
    this->mySurface = Handle_Geom_RectangularTrimmedSurface::DownCast(s->Copy());
}

GeomTrimmedSurface::~GeomTrimmedSurface()
{
}

void GeomTrimmedSurface::setHandle(const Handle_Geom_RectangularTrimmedSurface& s)
{
    mySurface = Handle_Geom_RectangularTrimmedSurface::DownCast(s->Copy());
}

const Handle_Geom_Geometry& GeomTrimmedSurface::handle() const
{
    return mySurface;
}

Geometry *GeomTrimmedSurface::clone(void) const
{
    GeomTrimmedSurface *newSurf = new GeomTrimmedSurface(mySurface);
    newSurf->Construction = this->Construction;
    return newSurf;
}

// Persistence implementer 
unsigned int GeomTrimmedSurface::getMemSize (void) const {assert(0); return 0;/* not implemented yet */}
void         GeomTrimmedSurface::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomTrimmedSurface::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomTrimmedSurface::getPyObject(void)
{
    return 0;
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomSurfaceOfRevolution,Part::GeomSurface);

GeomSurfaceOfRevolution::GeomSurfaceOfRevolution()
{
}

GeomSurfaceOfRevolution::GeomSurfaceOfRevolution(const Handle_Geom_Curve& c, const gp_Ax1& a)
{
    this->mySurface = new Geom_SurfaceOfRevolution(c,a);
}

GeomSurfaceOfRevolution::GeomSurfaceOfRevolution(const Handle_Geom_SurfaceOfRevolution& s)
{
    this->mySurface = Handle_Geom_SurfaceOfRevolution::DownCast(s->Copy());
}

GeomSurfaceOfRevolution::~GeomSurfaceOfRevolution()
{
}

void GeomSurfaceOfRevolution::setHandle(const Handle_Geom_SurfaceOfRevolution& c)
{
    mySurface = Handle_Geom_SurfaceOfRevolution::DownCast(c->Copy());
}

const Handle_Geom_Geometry& GeomSurfaceOfRevolution::handle() const
{
    return mySurface;
}

Geometry *GeomSurfaceOfRevolution::clone(void) const
{
    GeomSurfaceOfRevolution *newSurf = new GeomSurfaceOfRevolution(mySurface);
    newSurf->Construction = this->Construction;
    return newSurf;
}

// Persistence implementer 
unsigned int GeomSurfaceOfRevolution::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomSurfaceOfRevolution::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomSurfaceOfRevolution::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomSurfaceOfRevolution::getPyObject(void)
{
    return new SurfaceOfRevolutionPy((GeomSurfaceOfRevolution*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomSurfaceOfExtrusion,Part::GeomSurface);

GeomSurfaceOfExtrusion::GeomSurfaceOfExtrusion()
{
}

GeomSurfaceOfExtrusion::GeomSurfaceOfExtrusion(const Handle_Geom_Curve& c, const gp_Dir& d)
{
    this->mySurface = new Geom_SurfaceOfLinearExtrusion(c,d);
}

GeomSurfaceOfExtrusion::GeomSurfaceOfExtrusion(const Handle_Geom_SurfaceOfLinearExtrusion& s)
{
    this->mySurface = Handle_Geom_SurfaceOfLinearExtrusion::DownCast(s->Copy());
}

GeomSurfaceOfExtrusion::~GeomSurfaceOfExtrusion()
{
}

void GeomSurfaceOfExtrusion::setHandle(const Handle_Geom_SurfaceOfLinearExtrusion& c)
{
    mySurface = Handle_Geom_SurfaceOfLinearExtrusion::DownCast(c->Copy());
}

const Handle_Geom_Geometry& GeomSurfaceOfExtrusion::handle() const
{
    return mySurface;
}

Geometry *GeomSurfaceOfExtrusion::clone(void) const
{
    GeomSurfaceOfExtrusion *newSurf = new GeomSurfaceOfExtrusion(mySurface);
    newSurf->Construction = this->Construction;
    return newSurf;
}

// Persistence implementer 
unsigned int GeomSurfaceOfExtrusion::getMemSize (void) const               {assert(0); return 0;/* not implemented yet */}
void         GeomSurfaceOfExtrusion::Save       (Base::Writer &/*writer*/) const {assert(0);          /* not implemented yet */}
void         GeomSurfaceOfExtrusion::Restore    (Base::XMLReader &/*reader*/)    {assert(0);          /* not implemented yet */}

PyObject *GeomSurfaceOfExtrusion::getPyObject(void)
{
    return new SurfaceOfExtrusionPy((GeomSurfaceOfExtrusion*)this->clone());
}


// Helper functions for fillet tools
// -------------------------------------------------
namespace Part {

bool find2DLinesIntersection(const Base::Vector3d &orig1, const Base::Vector3d &dir1,
                             const Base::Vector3d &orig2, const Base::Vector3d &dir2,
                             Base::Vector3d &point)
{
    double det = dir1.x*dir2.y - dir1.y*dir2.x;
    if ((det > 0 ? det : -det) < 1e-10)
        return false;
    double c1 = dir1.y*orig1.x - dir1.x*orig1.y;
    double c2 = dir2.y*orig2.x - dir2.x*orig2.y;
    double x = (dir1.x*c2 - dir2.x*c1)/det;
    double y = (dir1.y*c2 - dir2.y*c1)/det;
    point = Base::Vector3d(x,y,0.f);
    return true;
}

bool find2DLinesIntersection(const GeomLineSegment *lineSeg1, const GeomLineSegment *lineSeg2,
                             Base::Vector3d &point)
{
    Base::Vector3d orig1 = lineSeg1->getStartPoint();
    Base::Vector3d orig2 = lineSeg2->getStartPoint();
    Base::Vector3d dir1 = (lineSeg1->getEndPoint()-lineSeg1->getStartPoint());
    Base::Vector3d dir2 = (lineSeg2->getEndPoint()-lineSeg2->getStartPoint());
    return find2DLinesIntersection(orig1, dir1, orig2, dir2, point);
}

bool findFilletCenter(const GeomLineSegment *lineSeg1, const GeomLineSegment *lineSeg2, double radius,
                      Base::Vector3d &center)
{
    Base::Vector3d midPoint1 = (lineSeg1->getStartPoint()+lineSeg1->getEndPoint())/2;
    Base::Vector3d midPoint2 = (lineSeg2->getStartPoint()+lineSeg2->getEndPoint())/2;
    return findFilletCenter(lineSeg1, lineSeg2, radius, midPoint1, midPoint2, center);
}

bool findFilletCenter(const GeomLineSegment *lineSeg1, const GeomLineSegment *lineSeg2, double radius,
                      const Base::Vector3d &refPnt1, const Base::Vector3d &refPnt2, Base::Vector3d &center)
{
    //Calculate directions and normals for each straight line
    Base::Vector3d l1p1, l1p2, l2p1, l2p2, dir1, dir2, norm1, norm2;
    l1p1 = lineSeg1->getStartPoint();
    l1p2 = lineSeg1->getEndPoint();
    l2p1 = lineSeg2->getStartPoint();
    l2p2 = lineSeg2->getEndPoint();

    dir1 = (l1p1 - l1p2).Normalize();
    dir2 = (l2p1 - l2p2).Normalize();

    norm1 = Base::Vector3d(dir1.y, -dir1.x, 0.).Normalize();
    norm2 = Base::Vector3d(dir2.y, -dir2.x, 0.).Normalize();

    // calculate the intersections between the normals to find inwards direction

    // find intersection of lines
    Base::Vector3d corner;
    if (!find2DLinesIntersection(lineSeg1,lineSeg2,corner))
        return false;

    // Just project the given reference points onto the lines, just in case they are not already lying on
    Base::Vector3d normPnt1, normPnt2;
    normPnt1.ProjToLine(refPnt1-l1p1, l1p2-l1p1);
    normPnt2.ProjToLine(refPnt2-l2p1, l2p2-l2p1);
    normPnt1 += refPnt1;
    normPnt2 += refPnt2;

    //Angle bisector
    Base::Vector3d bisectDir = ((normPnt1 - corner).Normalize() + (normPnt2-corner).Normalize()).Normalize();

    //redefine norms pointing towards bisect line
    Base::Vector3d normIntersection1, normIntersection2;
    if (find2DLinesIntersection(normPnt1, norm1, corner, bisectDir, normIntersection1) &&
        find2DLinesIntersection(normPnt2, norm2, corner, bisectDir, normIntersection2)) {
        norm1 = (normIntersection1 - normPnt1).Normalize();
        norm2 = (normIntersection2 - normPnt2).Normalize();
    } else {
        return false;
    }

    // Project lines to find mid point of fillet arc
    Base::Vector3d tmpPoint1 = l1p1 + (norm1 * radius);
    Base::Vector3d tmpPoint2 = l2p1 + (norm2 * radius);

    // found center point
    if (find2DLinesIntersection(tmpPoint1, dir1, tmpPoint2, dir2, center))
        return true;
    else
        return false;
}

// Returns -1 if radius cannot be suggested
double suggestFilletRadius(const GeomLineSegment *lineSeg1, const GeomLineSegment *lineSeg2,
                           const Base::Vector3d &refPnt1, const Base::Vector3d &refPnt2)
{
    Base::Vector3d corner;
    if (!Part::find2DLinesIntersection(lineSeg1, lineSeg2, corner))
        return -1;

    Base::Vector3d dir1 = lineSeg1->getEndPoint() - lineSeg1->getStartPoint();
    Base::Vector3d dir2 = lineSeg2->getEndPoint() - lineSeg2->getStartPoint();

    // Decide the line directions depending on the reference points
    if (dir1*(refPnt1-corner) < 0)
        dir1 *= -1;
    if (dir2*(refPnt2-corner) < 0)
        dir2 *= -1;

    //Angle bisector
    Base::Vector3d dirBisect = (dir1.Normalize() + dir2.Normalize()).Normalize();

    Base::Vector3d projPnt1, projPnt2;
    projPnt1.ProjToLine(refPnt1-corner, dir1);
    projPnt2.ProjToLine(refPnt2-corner, dir2);
    projPnt1 += refPnt1;
    projPnt2 += refPnt2;

    Base::Vector3d norm1(dir1.y, -dir1.x, 0.f);
    Base::Vector3d norm2(dir2.y, -dir2.x, 0.f);

    double r1=-1, r2=-1;
    Base::Vector3d center1, center2;
    if (find2DLinesIntersection(projPnt1, norm1, corner, dirBisect, center1))
        r1 = (projPnt1 - center1).Length();
    if (find2DLinesIntersection(projPnt2, norm2, corner, dirBisect, center2))
        r2 = (projPnt1 - center2).Length();

    return r1 < r2 ? r1 : r2;
}

GeomArcOfCircle *createFilletGeometry(const GeomLineSegment *lineSeg1, const GeomLineSegment *lineSeg2,
                                      const Base::Vector3d &center, double radius)
{
    Base::Vector3d corner;
    if (!Part::find2DLinesIntersection(lineSeg1, lineSeg2, corner))
        // Parallel Lines so return null pointer
        return 0;

    Base::Vector3d dir1 = lineSeg1->getEndPoint() - lineSeg1->getStartPoint();
    Base::Vector3d dir2 = lineSeg2->getEndPoint() - lineSeg2->getStartPoint();

    Base::Vector3d radDir1, radDir2;
    radDir1.ProjToLine(center - corner, dir1);
    radDir2.ProjToLine(center - corner, dir2);

    // Angle Variables
    double startAngle, endAngle, range;

    startAngle = atan2(radDir1.y, radDir1.x);
    range = atan2(-radDir1.y*radDir2.x+radDir1.x*radDir2.y,
                   radDir1.x*radDir2.x+radDir1.y*radDir2.y);
    endAngle = startAngle + range;

    if (endAngle < startAngle)
        std::swap(startAngle, endAngle);

    if (endAngle > 2*M_PI )
        endAngle -= 2*M_PI;

    if (startAngle < 0 )
        endAngle += 2*M_PI;

    // Create Arc Segment
    GeomArcOfCircle *arc = new GeomArcOfCircle();
    arc->setRadius(radius);
    arc->setCenter(center);
    arc->setRange(startAngle, endAngle);

    return arc;
}

}
