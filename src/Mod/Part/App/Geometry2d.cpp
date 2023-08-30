/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepBuilderAPI_MakeEdge2d.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <Geom2dAPI_Interpolate.hxx>
# include <Geom2dAPI_ProjectPointOnCurve.hxx>
# include <Geom2dConvert.hxx>
# include <Geom2dConvert_CompCurveToBSplineCurve.hxx>
# include <Geom2dLProp_CLProps2d.hxx>
# include <gce_ErrorType.hxx>
# include <gp_Ax22d.hxx>
# include <gp_Circ2d.hxx>
# include <gp_Elips2d.hxx>
# include <gp_Hypr2d.hxx>
# include <gp_Lin2d.hxx>
# include <gp_Parab2d.hxx>
# include <Standard_ConstructionError.hxx>
# include <Standard_Version.hxx>
# include <TColgp_Array1OfPnt2d.hxx>
# include <TColgp_Array1OfVec2d.hxx>
# include <TColgp_HArray1OfPnt2d.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColStd_HArray1OfBoolean.hxx>
# include <GCE2d_MakeArcOfCircle.hxx>
# include <GCE2d_MakeArcOfEllipse.hxx>
# include <GCE2d_MakeArcOfHyperbola.hxx>
# include <GCE2d_MakeArcOfParabola.hxx>
# include <GCE2d_MakeCircle.hxx>
# include <GCE2d_MakeEllipse.hxx>
# include <GCE2d_MakeHyperbola.hxx>
# include <GCE2d_MakeLine.hxx>
# include <GCE2d_MakeParabola.hxx>
# include <GCE2d_MakeSegment.hxx>
# include <Precision.hxx>
#endif

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "Geometry2d.h"

#include <Geom2d/ArcOfCircle2dPy.h>
#include <Geom2d/ArcOfEllipse2dPy.h>
#include <Geom2d/ArcOfHyperbola2dPy.h>
#include <Geom2d/ArcOfParabola2dPy.h>
#include <Geom2d/BezierCurve2dPy.h>
#include <Geom2d/BSplineCurve2dPy.h>
#include <Geom2d/Circle2dPy.h>
#include <Geom2d/Ellipse2dPy.h>
#include <Geom2d/Hyperbola2dPy.h>
#include <Geom2d/Line2dSegmentPy.h>
#include <Geom2d/Line2dPy.h>
#include <Geom2d/OffsetCurve2dPy.h>
#include <Geom2d/Parabola2dPy.h>


using namespace Part;
using namespace std;

extern const char* gce_ErrorStatusText(gce_ErrorType et);


TYPESYSTEM_SOURCE_ABSTRACT(Part::Geometry2d, Base::Persistence)

Geometry2d::Geometry2d() = default;

Geometry2d::~Geometry2d() = default;

unsigned int Geometry2d::getMemSize () const
{
    return sizeof(Geometry2d);
}

void Geometry2d::Save(Base::Writer & /*writer*/) const
{
    throw Base::NotImplementedError("Save");
}

void Geometry2d::Restore(Base::XMLReader & /*reader*/)
{
    throw Base::NotImplementedError("Restore");
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dPoint, Part::Geometry2d)

Geom2dPoint::Geom2dPoint()
{
    this->myPoint = new Geom2d_CartesianPoint(0,0);
}

Geom2dPoint::Geom2dPoint(const Handle(Geom2d_CartesianPoint)& p)
{
    this->myPoint = Handle(Geom2d_CartesianPoint)::DownCast(p->Copy());
}

Geom2dPoint::Geom2dPoint(const Base::Vector2d& p)
{
    this->myPoint = new Geom2d_CartesianPoint(p.x,p.y);
}

Geom2dPoint::~Geom2dPoint() = default;

TopoDS_Shape Geom2dPoint::toShape() const
{
    Handle(Geom2d_CartesianPoint) c = Handle(Geom2d_CartesianPoint)::DownCast(handle());
    gp_Pnt2d xy = c->Pnt2d();
    gp_Pnt pnt;
    pnt.SetX(xy.X());
    pnt.SetY(xy.Y());
    BRepBuilderAPI_MakeVertex mkBuilder(pnt);
    return mkBuilder.Shape();
}

const Handle(Geom2d_Geometry)& Geom2dPoint::handle() const
{
    return myPoint;
}

Geometry2d *Geom2dPoint::clone() const
{
    Geom2dPoint *newPoint = new Geom2dPoint(myPoint);
    return newPoint;
}

Base::Vector2d Geom2dPoint::getPoint()const
{
    return {myPoint->X(),myPoint->Y()};
}

void Geom2dPoint::setPoint(const Base::Vector2d& p)
{
    this->myPoint->SetCoord(p.x,p.y);
}

unsigned int Geom2dPoint::getMemSize () const
{
    return sizeof(Geom2d_CartesianPoint);
}

void Geom2dPoint::Save(Base::Writer &writer) const
{
    // save the attributes of the father class
    Geometry2d::Save(writer);

    Base::Vector2d Point   =  getPoint();
    writer.Stream()
        << writer.ind()
        << "<Geom2dPoint "
        << "X=\"" << Point.x << "\" "
        << "Y=\"" << Point.y << "\" "
        << "/>" << endl;
}

void Geom2dPoint::Restore(Base::XMLReader &reader)
{
    // read the attributes of the father class
    Geometry2d::Restore(reader);

    double X,Y;
    // read my Element
    reader.readElement("Geom2dPoint");
    // get the value of my Attribute
    X = reader.getAttributeAsFloat("X");
    Y = reader.getAttributeAsFloat("Y");

    // set the read geometry
    setPoint(Base::Vector2d(X,Y));
}

PyObject *Geom2dPoint::getPyObject()
{
    Handle(Geom2d_CartesianPoint) c = Handle(Geom2d_CartesianPoint)::DownCast(handle());
    gp_Pnt2d xy = c->Pnt2d();

    Py::Tuple tuple(2);
    tuple.setItem(0, Py::Float(xy.X()));
    tuple.setItem(1, Py::Float(xy.Y()));
    return Py::new_reference_to(tuple);
}

// -------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Part::Geom2dCurve, Part::Geometry2d)

Geom2dCurve::Geom2dCurve() = default;

Geom2dCurve::~Geom2dCurve() = default;

TopoDS_Shape Geom2dCurve::toShape() const
{
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(handle());
    BRepBuilderAPI_MakeEdge2d mkBuilder(c);
    return mkBuilder.Shape();
}

bool Geom2dCurve::tangent(double u, gp_Dir2d& dir) const
{
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(handle());
    Geom2dLProp_CLProps2d prop(c,u,2,Precision::Confusion());
    if (prop.IsTangentDefined()) {
        prop.Tangent(dir);
        return true;
    }

    return false;
}

Base::Vector2d Geom2dCurve::pointAtParameter(double u) const
{
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(handle());
    Geom2dLProp_CLProps2d prop(c,u,0,Precision::Confusion());

    const gp_Pnt2d &point=prop.Value();
    return {point.X(),point.Y()};
}

Base::Vector2d Geom2dCurve::firstDerivativeAtParameter(double u) const
{
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(handle());
    Geom2dLProp_CLProps2d prop(c,u,1,Precision::Confusion());

    const gp_Vec2d &vec=prop.D1();
    return {vec.X(),vec.Y()};
}

Base::Vector2d Geom2dCurve::secondDerivativeAtParameter(double u) const
{
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(handle());
    Geom2dLProp_CLProps2d prop(c,u,2,Precision::Confusion());

    const gp_Vec2d &vec=prop.D2();
    return {vec.X(),vec.Y()};
}

bool Geom2dCurve::normal(double u, gp_Dir2d& dir) const
{
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(handle());
    Geom2dLProp_CLProps2d prop(c,u,2,Precision::Confusion());
    if (prop.IsTangentDefined()) {
        prop.Normal(dir);
        return true;
    }

    return false;
}

bool Geom2dCurve::closestParameter(const Base::Vector2d& point, double &u) const
{
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(handle());
    try {
        if (!c.IsNull()) {
            gp_Pnt2d pnt(point.x,point.y);
            Geom2dAPI_ProjectPointOnCurve ppc(pnt, c);
            u = ppc.LowerDistanceParameter();
            return true;
        }
    }
    catch (Standard_Failure& e) {

        std::cout << e.GetMessageString() << std::endl;
        return false;
    }

    return false;
}

bool Geom2dCurve::closestParameterToBasicCurve(const Base::Vector2d& point, double &u) const
{
    Handle(Geom2d_Curve) c = Handle(Geom2d_Curve)::DownCast(handle());

    if (c->IsKind(STANDARD_TYPE(Geom2d_TrimmedCurve))){
        Handle(Geom2d_TrimmedCurve) tc = Handle(Geom2d_TrimmedCurve)::DownCast(handle());
        Handle(Geom2d_Curve) bc = tc->BasisCurve();
        try {
            if (!bc.IsNull()) {
                gp_Pnt2d pnt(point.x,point.y);
                Geom2dAPI_ProjectPointOnCurve ppc(pnt, bc);
                u = ppc.LowerDistanceParameter();
                return true;
            }
        }
        catch (Standard_Failure& e) {

            std::cout << e.GetMessageString() << std::endl;
            return false;
        }

        return false;

    }
    else {
        return this->closestParameter(point, u);
    }
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dBezierCurve, Part::Geom2dCurve)

Geom2dBezierCurve::Geom2dBezierCurve()
{
    TColgp_Array1OfPnt2d poles(1,2);
    poles(1) = gp_Pnt2d(0.0,0.0);
    poles(2) = gp_Pnt2d(0.0,1.0);
    Handle(Geom2d_BezierCurve) b = new Geom2d_BezierCurve(poles);
    this->myCurve = b;
}

Geom2dBezierCurve::Geom2dBezierCurve(const Handle(Geom2d_BezierCurve)& b)
{
    this->myCurve = Handle(Geom2d_BezierCurve)::DownCast(b->Copy());
}

Geom2dBezierCurve::~Geom2dBezierCurve() = default;

void Geom2dBezierCurve::setHandle(const Handle(Geom2d_BezierCurve)& c)
{
    myCurve = Handle(Geom2d_BezierCurve)::DownCast(c->Copy());
}

const Handle(Geom2d_Geometry)& Geom2dBezierCurve::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dBezierCurve::clone() const
{
    Geom2dBezierCurve *newCurve = new Geom2dBezierCurve(myCurve);
    return newCurve;
}

unsigned int Geom2dBezierCurve::getMemSize () const
{
    throw Base::NotImplementedError("Geom2dBezierCurve::getMemSize");
}

void Geom2dBezierCurve::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("Geom2dBezierCurve::Save");
}

void Geom2dBezierCurve::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("Geom2dBezierCurve::Restore");
}

PyObject *Geom2dBezierCurve::getPyObject()
{
    return new BezierCurve2dPy(static_cast<Geom2dBezierCurve*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dBSplineCurve, Part::Geom2dCurve)

Geom2dBSplineCurve::Geom2dBSplineCurve()
{
    TColgp_Array1OfPnt2d poles(1,2);
    poles(1) = gp_Pnt2d(0.0,0.0);
    poles(2) = gp_Pnt2d(1.0,0.0);

    TColStd_Array1OfReal knots(1,2);
    knots(1) = 0.0;
    knots(2) = 1.0;

    TColStd_Array1OfInteger mults(1,2);
    mults(1) = 2;
    mults(2) = 2;

    this->myCurve = new Geom2d_BSplineCurve(poles, knots, mults, 1);
}

Geom2dBSplineCurve::Geom2dBSplineCurve(const Handle(Geom2d_BSplineCurve)& b)
{
    this->myCurve = Handle(Geom2d_BSplineCurve)::DownCast(b->Copy());
}

Geom2dBSplineCurve::~Geom2dBSplineCurve() = default;

void Geom2dBSplineCurve::setHandle(const Handle(Geom2d_BSplineCurve)& c)
{
    myCurve = Handle(Geom2d_BSplineCurve)::DownCast(c->Copy());
}

const Handle(Geom2d_Geometry)& Geom2dBSplineCurve::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dBSplineCurve::clone() const
{
    Geom2dBSplineCurve *newCurve = new Geom2dBSplineCurve(myCurve);
    return newCurve;
}

int Geom2dBSplineCurve::countPoles() const
{
    return myCurve->NbPoles();
}

void Geom2dBSplineCurve::setPole(int index, const Base::Vector2d& pole, double weight)
{
    try {
        gp_Pnt2d pnt(pole.x,pole.y);
        if (weight < 0.0)
            myCurve->SetPole(index+1,pnt);
        else
            myCurve->SetPole(index+1,pnt,weight);
    }
    catch (Standard_Failure& e) {

        std::cout << e.GetMessageString() << std::endl;
    }
}

std::vector<Base::Vector2d> Geom2dBSplineCurve::getPoles() const
{
    std::vector<Base::Vector2d> poles;
    poles.reserve(myCurve->NbPoles());
    TColgp_Array1OfPnt2d p(1,myCurve->NbPoles());
    myCurve->Poles(p);

    for (Standard_Integer i=p.Lower(); i<=p.Upper(); i++) {
        const gp_Pnt2d& pnt = p(i);
        poles.emplace_back(pnt.X(), pnt.Y());
    }
    return poles;
}

bool Geom2dBSplineCurve::join(const Handle(Geom2d_BSplineCurve)& spline)
{
    Geom2dConvert_CompCurveToBSplineCurve ccbc(this->myCurve);
    if (!ccbc.Add(spline, Precision::Approximation()))
        return false;
    this->myCurve = ccbc.BSplineCurve();
    return true;
}

void Geom2dBSplineCurve::interpolate(const std::vector<gp_Pnt2d>& p,
                                     const std::vector<gp_Vec2d>& t)
{
    if (p.size() < 2)
        Standard_ConstructionError::Raise();
    if (p.size() != t.size())
        Standard_ConstructionError::Raise();

    double tol3d = Precision::Approximation();
    Handle(TColgp_HArray1OfPnt2d) pts = new TColgp_HArray1OfPnt2d(1, p.size());
    for (std::size_t i=0; i<p.size(); i++) {
        pts->SetValue(i+1, p[i]);
    }

    TColgp_Array1OfVec2d tgs(1, t.size());
    Handle(TColStd_HArray1OfBoolean) fgs = new TColStd_HArray1OfBoolean(1, t.size());
    for (std::size_t i=0; i<p.size(); i++) {
        tgs.SetValue(i+1, t[i]);
        fgs->SetValue(i+1, Standard_True);
    }

    Geom2dAPI_Interpolate interpolate(pts, Standard_False, tol3d);
    interpolate.Load(tgs, fgs);
    interpolate.Perform();
    this->myCurve = interpolate.Curve();
}

void Geom2dBSplineCurve::getCardinalSplineTangents(const std::vector<gp_Pnt2d>& p,
                                                   const std::vector<double>& c,
                                                   std::vector<gp_Vec2d>& t) const
{
    // https://de.wikipedia.org/wiki/Kubisch_Hermitescher_Spline#Cardinal_Spline
    if (p.size() < 2)
        Standard_ConstructionError::Raise();
    if (p.size() != c.size())
        Standard_ConstructionError::Raise();

    t.resize(p.size());
    if (p.size() == 2) {
        t[0] = gp_Vec2d(p[0], p[1]);
        t[1] = gp_Vec2d(p[0], p[1]);
    }
    else {
        std::size_t e = p.size() - 1;

        for (std::size_t i = 1; i < e; i++) {
            gp_Vec2d v = gp_Vec2d(p[i-1], p[i+1]);
            double f = 0.5 * (1-c[i]);
            v.Scale(f);
            t[i] = v;
        }

        t[0] = t[1];
        t[t.size()-1] = t[t.size()-2];
    }
}

void Geom2dBSplineCurve::getCardinalSplineTangents(const std::vector<gp_Pnt2d>& p, double c,
                                                   std::vector<gp_Vec2d>& t) const
{
    // https://de.wikipedia.org/wiki/Kubisch_Hermitescher_Spline#Cardinal_Spline
    if (p.size() < 2)
        Standard_ConstructionError::Raise();

    t.resize(p.size());
    if (p.size() == 2) {
        t[0] = gp_Vec2d(p[0], p[1]);
        t[1] = gp_Vec2d(p[0], p[1]);
    }
    else {
        std::size_t e = p.size() - 1;
        double f = 0.5 * (1-c);

        for (std::size_t i = 1; i < e; i++) {
            gp_Vec2d v = gp_Vec2d(p[i-1], p[i+1]);
            v.Scale(f);
            t[i] = v;
        }

        t[0] = t[1];
        t[t.size()-1] = t[t.size()-2];
    }
}

void Geom2dBSplineCurve::makeC1Continuous(double tol)
{
    Geom2dConvert::C0BSplineToC1BSplineCurve(this->myCurve, tol);
}

std::list<Geometry2d*> Geom2dBSplineCurve::toBiArcs(double /*tolerance*/) const
{
    Standard_Failure::Raise("Not yet implemented");
    return {};
}

unsigned int Geom2dBSplineCurve::getMemSize() const
{
    throw Base::NotImplementedError("Geom2dBSplineCurve::getMemSize");
}

void Geom2dBSplineCurve::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("Geom2dBSplineCurve::Save");
}

void Geom2dBSplineCurve::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("Geom2dBSplineCurve::Restore");
}

PyObject *Geom2dBSplineCurve::getPyObject()
{
    return new BSplineCurve2dPy(static_cast<Geom2dBSplineCurve*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Part::Geom2dConic, Part::Geom2dCurve)

Geom2dConic::Geom2dConic() = default;

Geom2dConic::~Geom2dConic() = default;

Base::Vector2d Geom2dConic::getLocation() const
{
    Handle(Geom2d_Conic) conic = Handle(Geom2d_Conic)::DownCast(handle());
    const gp_Pnt2d& loc = conic->Location();
    return {loc.X(),loc.Y()};
}

void Geom2dConic::setLocation(const Base::Vector2d& Center)
{
    gp_Pnt2d p1(Center.x,Center.y);
    Handle(Geom2d_Conic) conic = Handle(Geom2d_Conic)::DownCast(handle());

    try {
        conic->SetLocation(p1);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

bool Geom2dConic::isReversed() const
{
    Handle(Geom2d_Conic) conic = Handle(Geom2d_Conic)::DownCast(handle());
    gp_Dir2d xdir = conic->XAxis().Direction();
    gp_Dir2d ydir = conic->YAxis().Direction();

    Base::Vector3d xd(xdir.X(), xdir.Y(), 0);
    Base::Vector3d yd(ydir.X(), ydir.Y(), 0);
    Base::Vector3d zd = xd.Cross(yd);
    return zd.z < 0;
}

void Geom2dConic::SaveAxis(Base::Writer& writer, const gp_Ax22d& axis) const
{
    gp_Pnt2d center = axis.Location();
    gp_Dir2d xdir = axis.XDirection();
    gp_Dir2d ydir = axis.YDirection();
    writer.Stream()
        << "CenterX=\"" << center.X() << "\" "
        << "CenterY=\"" << center.Y() << "\" "
        << "XAxisX=\"" << xdir.X() << "\" "
        << "XAxisY=\"" << xdir.Y() << "\" "
        << "YAxisX=\"" << ydir.X() << "\" "
        << "YAxisY=\"" << ydir.Y() << "\" ";
}

void Geom2dConic::RestoreAxis(Base::XMLReader& reader, gp_Ax22d& axis)
{
    double CenterX,CenterY,XdirX,XdirY,YdirX,YdirY;
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    XdirX = reader.getAttributeAsFloat("XAxisX");
    XdirY = reader.getAttributeAsFloat("XAxisY");
    YdirX = reader.getAttributeAsFloat("YAxisX");
    YdirY = reader.getAttributeAsFloat("YAxisY");

    // set the read geometry
    gp_Pnt2d p1(CenterX,CenterY);
    gp_Dir2d xdir(XdirX,XdirY);
    gp_Dir2d ydir(YdirX,YdirY);
    axis.SetLocation(p1);
    axis.SetXDirection(xdir);
    axis.SetYDirection(ydir);
}

// -------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Part::Geom2dArcOfConic, Part::Geom2dCurve)

Geom2dArcOfConic::Geom2dArcOfConic() = default;

Geom2dArcOfConic::~Geom2dArcOfConic() = default;

Base::Vector2d Geom2dArcOfConic::getLocation() const
{
    Handle(Geom2d_TrimmedCurve) curve = Handle(Geom2d_TrimmedCurve)::DownCast(handle());
    Handle(Geom2d_Conic) conic = Handle(Geom2d_Conic)::DownCast(curve->BasisCurve());
    const gp_Pnt2d& loc = conic->Location();
    return {loc.X(),loc.Y()};
}

void Geom2dArcOfConic::setLocation(const Base::Vector2d& Center)
{
    gp_Pnt2d p1(Center.x,Center.y);
    Handle(Geom2d_TrimmedCurve) curve = Handle(Geom2d_TrimmedCurve)::DownCast(handle());
    Handle(Geom2d_Conic) conic = Handle(Geom2d_Conic)::DownCast(curve->BasisCurve());

    try {
        conic->SetLocation(p1);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

bool Geom2dArcOfConic::isReversed() const
{
    Handle(Geom2d_TrimmedCurve) curve = Handle(Geom2d_TrimmedCurve)::DownCast(handle());
    Handle(Geom2d_Conic) conic = Handle(Geom2d_Conic)::DownCast(curve->BasisCurve());
    gp_Dir2d xdir = conic->XAxis().Direction();
    gp_Dir2d ydir = conic->YAxis().Direction();

    Base::Vector3d xd(xdir.X(), xdir.Y(), 0);
    Base::Vector3d yd(ydir.X(), ydir.Y(), 0);
    Base::Vector3d zd = xd.Cross(yd);
    return zd.z < 0;
}

/*!
 * \brief Geom2dArcOfConic::getStartPoint
 * \return XY of the arc's starting point.
 */
Base::Vector2d Geom2dArcOfConic::getStartPoint() const
{
    Handle(Geom2d_TrimmedCurve) curve = Handle(Geom2d_TrimmedCurve)::DownCast(handle());
    gp_Pnt2d pnt = curve->StartPoint();
    return {pnt.X(), pnt.Y()};
}

/*!
 * \brief Geom2dArcOfConic::getEndPoint
 * \return XY of the arc's ending point.
 */
Base::Vector2d Geom2dArcOfConic::getEndPoint() const
{
    Handle(Geom2d_TrimmedCurve) curve = Handle(Geom2d_TrimmedCurve)::DownCast(handle());
    gp_Pnt2d pnt = curve->EndPoint();
    return {pnt.X(), pnt.Y()};
}

/*!
 * \brief Geom2dArcOfConic::getRange
 * \param u [out] start angle of the arc, in radians.
 * \param v [out] end angle of the arc, in radians.
 */
void Geom2dArcOfConic::getRange(double& u, double& v) const
{
    Handle(Geom2d_TrimmedCurve) curve = Handle(Geom2d_TrimmedCurve)::DownCast(handle());
    u = curve->FirstParameter();
    v = curve->LastParameter();
}

/*!
 * \brief Geom2dArcOfConic::setRange
 * \param u [in] start angle of the arc, in radians.
 * \param v [in] end angle of the arc, in radians.
 */
void Geom2dArcOfConic::setRange(double u, double v)
{
    try {
        Handle(Geom2d_TrimmedCurve) curve = Handle(Geom2d_TrimmedCurve)::DownCast(handle());
        curve->SetTrim(u, v);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

void Geom2dArcOfConic::SaveAxis(Base::Writer& writer, const gp_Ax22d& axis, double u, double v) const
{
    gp_Pnt2d center = axis.Location();
    gp_Dir2d xdir = axis.XDirection();
    gp_Dir2d ydir = axis.YDirection();
    writer.Stream()
        << "CenterX=\"" << center.X() << "\" "
        << "CenterY=\"" << center.Y() << "\" "
        << "XAxisX=\"" << xdir.X() << "\" "
        << "XAxisY=\"" << xdir.Y() << "\" "
        << "YAxisX=\"" << ydir.X() << "\" "
        << "YAxisY=\"" << ydir.Y() << "\" "
        << "FirstParameter=\"" << u << "\" "
        << "LastParameter=\"" << v << "\" ";
}

void Geom2dArcOfConic::RestoreAxis(Base::XMLReader& reader, gp_Ax22d& axis, double& u, double &v)
{
    double CenterX,CenterY,XdirX,XdirY,YdirX,YdirY;
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    XdirX = reader.getAttributeAsFloat("XAxisX");
    XdirY = reader.getAttributeAsFloat("XAxisY");
    YdirX = reader.getAttributeAsFloat("YAxisX");
    YdirY = reader.getAttributeAsFloat("YAxisY");
    u = reader.getAttributeAsFloat("FirstParameter");
    v = reader.getAttributeAsFloat("LastParameter");

    // set the read geometry
    gp_Pnt2d p1(CenterX,CenterY);
    gp_Dir2d xdir(XdirX,XdirY);
    gp_Dir2d ydir(YdirX,YdirY);
    axis.SetLocation(p1);
    axis.SetXDirection(xdir);
    axis.SetYDirection(ydir);
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dCircle, Part::Geom2dConic)

Geom2dCircle::Geom2dCircle()
{
    Handle(Geom2d_Circle) c = new Geom2d_Circle(gp_Circ2d());
    this->myCurve = c;
}

Geom2dCircle::Geom2dCircle(const Handle(Geom2d_Circle)& c)
{
    this->myCurve = Handle(Geom2d_Circle)::DownCast(c->Copy());
}

Geom2dCircle::~Geom2dCircle() = default;

const Handle(Geom2d_Geometry)& Geom2dCircle::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dCircle::clone() const
{
    Geom2dCircle *newCirc = new Geom2dCircle(myCurve);
    return newCirc;
}

double Geom2dCircle::getRadius() const
{
    Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(handle());
    return circle->Radius();
}

void Geom2dCircle::setRadius(double Radius)
{
    Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(handle());

    try {
        gp_Circ2d c = circle->Circ2d();
        c.SetRadius(Radius);
        circle->SetCirc2d(c);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

unsigned int Geom2dCircle::getMemSize () const
{
    return sizeof(Geom2d_Circle);
}

void Geom2dCircle::Save(Base::Writer& writer) const
{
    // save the attributes of the father class
    Geom2dCurve::Save(writer);

    Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(handle());
    gp_Circ2d c = circle->Circ2d();
    gp_Ax22d axis = c.Axis();

    writer.Stream()
        << writer.ind()
        << "<Geom2dCircle ";
    SaveAxis(writer, axis);
    writer.Stream()
        << "Radius=\"" << c.Radius() << "\" "
        << "/>" << endl;
}

void Geom2dCircle::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    Geom2dCurve::Restore(reader);

    double Radius;
    gp_Ax22d axis;
    // read my Element
    reader.readElement("Geom2dCircle");
    // get the value of my Attribute
    RestoreAxis(reader, axis);
    Radius = reader.getAttributeAsFloat("Radius");

    try {
        GCE2d_MakeCircle mc(axis, Radius);
        if (!mc.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(mc.Status()));

        this->myCurve = mc.Value();
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

PyObject *Geom2dCircle::getPyObject()
{
    return new Circle2dPy(static_cast<Geom2dCircle*>(this->clone()));
}

/*
Find the centerpoint of a circle drawn through any 3 points:

Given points p1-3, draw 2 lines: S12 and S23 which each connect two points.  From the
midpoint of each line, draw a perpendicular line (S12p/S23p) across the circle.  These
lines will cross at the centerpoint.

Mathematically, line S12 will have a slope of m12 which can be determined.  Therefore,
the slope m12p is -1/m12. Line S12p will have an equation of y = m12p*x + b12p.  b12p can
be solved for using the midpoint of the line.  This can be done for both lines.  Since
both S12p and S23p cross at the centerpoint, solving the two equations together will give
the location of the centerpoint.
*/
Base::Vector2d Geom2dCircle::getCircleCenter (const Base::Vector2d &p1, const Base::Vector2d &p2, const Base::Vector2d &p3)
{
    Base::Vector2d u = p2-p1;
    Base::Vector2d v = p3-p2;
    Base::Vector2d w = p1-p3;

    double uu =  u*u;
    double vv =  v*v;
    double ww =  w*w;

    double eps2 = Precision::SquareConfusion();
    if (uu < eps2 || vv < eps2 || ww < eps2)
        THROWM(Base::ValueError,"Two points are coincident");

    double uv = -(u*v);
    double vw = -(v*w);
    double uw = -(u*w);

    double w0 = (2 * sqrt(abs(uu * ww - uw * uw)) * uw / (uu * ww));
    double w1 = (2 * sqrt(abs(uu * vv - uv * uv)) * uv / (uu * vv));
    double w2 = (2 * sqrt(abs(vv * ww - vw * vw)) * vw / (vv * ww));

    double wx = w0 + w1 + w2;

    if (abs(wx) < Precision::Confusion())
        THROWM(Base::ValueError,"Points are collinear");

    double x = (w0*p1.x + w1*p2.x + w2*p3.x)/wx;
    double y = (w0*p1.y + w1*p2.y + w2*p3.y)/wx;

    return {x, y};
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dArcOfCircle, Part::Geom2dArcOfConic)

Geom2dArcOfCircle::Geom2dArcOfCircle()
{
    Handle(Geom2d_Circle) c = new Geom2d_Circle(gp_Circ2d());
    this->myCurve = new Geom2d_TrimmedCurve(c, c->FirstParameter(),c->LastParameter());
}

Geom2dArcOfCircle::Geom2dArcOfCircle(const Handle(Geom2d_Circle)& c)
{
    this->myCurve = new Geom2d_TrimmedCurve(c, c->FirstParameter(),c->LastParameter());
}

Geom2dArcOfCircle::~Geom2dArcOfCircle() = default;

void Geom2dArcOfCircle::setHandle(const Handle(Geom2d_TrimmedCurve)& c)
{
    Handle(Geom2d_Circle) basis = Handle(Geom2d_Circle)::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not a circle");
    this->myCurve = Handle(Geom2d_TrimmedCurve)::DownCast(c->Copy());
}

const Handle(Geom2d_Geometry)& Geom2dArcOfCircle::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dArcOfCircle::clone() const
{
    Geom2dArcOfCircle* copy = new Geom2dArcOfCircle();
    copy->setHandle(this->myCurve);
    return copy;
}

double Geom2dArcOfCircle::getRadius() const
{
    Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(myCurve->BasisCurve());
    return circle->Radius();
}

void Geom2dArcOfCircle::setRadius(double Radius)
{
    Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(myCurve->BasisCurve());

    try {
        gp_Circ2d c = circle->Circ2d();
        c.SetRadius(Radius);
        circle->SetCirc2d(c);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

unsigned int Geom2dArcOfCircle::getMemSize () const
{
    return sizeof(Geom2d_Circle) + 2 *sizeof(double);
}

void Geom2dArcOfCircle::Save(Base::Writer &writer) const
{
    // save the attributes of the father class
    Geom2dCurve::Save(writer);

    Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(this->myCurve->BasisCurve());

    gp_Circ2d c = circle->Circ2d();
    gp_Ax22d axis = c.Axis();
    double u = this->myCurve->FirstParameter();
    double v = this->myCurve->LastParameter();

    writer.Stream()
        << writer.ind()
        << "<Geom2dArcOfCircle ";
    SaveAxis(writer, axis, u, v);
    writer.Stream()
        << "Radius=\"" << c.Radius() << "\" "
        << "/>" << endl;
}

void Geom2dArcOfCircle::Restore(Base::XMLReader &reader)
{
    // read the attributes of the father class
    Geom2dCurve::Restore(reader);

    double Radius,u,v;
    gp_Ax22d axis;
    // read my Element
    reader.readElement("Geom2dArcOfCircle");
    // get the value of my Attribute
    RestoreAxis(reader, axis, u, v);
    Radius = reader.getAttributeAsFloat("Radius");

    try {
        GCE2d_MakeCircle mc(axis, Radius);
        if (!mc.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(mc.Status()));
        GCE2d_MakeArcOfCircle ma(mc.Value()->Circ2d(), u, v);
        if (!ma.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(ma.Status()));

        Handle(Geom2d_TrimmedCurve) tmpcurve = ma.Value();
        Handle(Geom2d_Circle) tmpcircle = Handle(Geom2d_Circle)::DownCast(tmpcurve->BasisCurve());
        Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(this->myCurve->BasisCurve());

        circle->SetCirc2d(tmpcircle->Circ2d());
        this->myCurve->SetTrim(tmpcurve->FirstParameter(), tmpcurve->LastParameter());
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

PyObject *Geom2dArcOfCircle::getPyObject()
{
    return new ArcOfCircle2dPy(static_cast<Geom2dArcOfCircle*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dEllipse, Part::Geom2dConic)

Geom2dEllipse::Geom2dEllipse()
{
    Handle(Geom2d_Ellipse) e = new Geom2d_Ellipse(gp_Elips2d());
    this->myCurve = e;
}

Geom2dEllipse::Geom2dEllipse(const Handle(Geom2d_Ellipse)& e)
{
    this->myCurve = Handle(Geom2d_Ellipse)::DownCast(e->Copy());
}

Geom2dEllipse::~Geom2dEllipse() = default;

const Handle(Geom2d_Geometry)& Geom2dEllipse::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dEllipse::clone() const
{
    Geom2dEllipse *newEllipse = new Geom2dEllipse(myCurve);
    return newEllipse;
}

double Geom2dEllipse::getMajorRadius() const
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(handle());
    return ellipse->MajorRadius();
}

void Geom2dEllipse::setMajorRadius(double Radius)
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(handle());

    try {
        ellipse->SetMajorRadius(Radius);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

double Geom2dEllipse::getMinorRadius() const
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(handle());
    return ellipse->MinorRadius();
}

void Geom2dEllipse::setMinorRadius(double Radius)
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(handle());

    try {
        ellipse->SetMinorRadius(Radius);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

/*!
 * \brief Geom2dEllipse::getMajorAxisDir
 * \return the direction vector (unit-length) of major axis of the ellipse. The
 * direction also points to the first focus.
 */
Base::Vector2d Geom2dEllipse::getMajorAxisDir() const
{
    gp_Dir2d xdir = myCurve->XAxis().Direction();
    return {xdir.X(), xdir.Y()};
}

/*!
 * \brief Geom2dEllipse::setMajorAxisDir Rotates the ellipse in its plane, so
 * that its major axis is as close as possible to the provided direction.
 * \param newdir [in] is the new direction. If the vector is small, the
 * orientation of the ellipse will be preserved. If the vector is not small,
 * but its projection onto plane of the ellipse is small, an exception will be
 * thrown.
 */
void Geom2dEllipse::setMajorAxisDir(Base::Vector2d newdir)
{
    if (newdir.Length() < Precision::Confusion())
        return;//zero vector was passed. Keep the old orientation.
    try {
        gp_Elips2d e = myCurve->Elips2d();
        gp_Ax22d pos = e.Axis();
        pos.SetXDirection(gp_Dir2d(newdir.x, newdir.y));
        e.SetAxis(pos);
        myCurve->SetElips2d(e);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

unsigned int Geom2dEllipse::getMemSize () const
{
    return sizeof(Geom2d_Ellipse);
}

void Geom2dEllipse::Save(Base::Writer& writer) const
{
    // save the attributes of the father class
    Geom2dCurve::Save(writer);

    gp_Elips2d e = this->myCurve->Elips2d();
    gp_Ax22d axis = e.Axis();

    writer.Stream()
        << writer.ind()
        << "<Geom2dEllipse ";
    SaveAxis(writer, axis);
    writer.Stream()
        << "MajorRadius=\"" << e.MajorRadius() << "\" "
        << "MinorRadius=\"" << e.MinorRadius() << "\" "
        << "/>" << endl;
}

void Geom2dEllipse::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    Geom2dCurve::Restore(reader);

    double MajorRadius,MinorRadius;
    gp_Ax22d axis;
    // read my Element
    reader.readElement("Geom2dEllipse");
    // get the value of my Attribute
    RestoreAxis(reader, axis);
    MajorRadius = reader.getAttributeAsFloat("MajorRadius");
    MinorRadius = reader.getAttributeAsFloat("MinorRadius");

    try {
        GCE2d_MakeEllipse mc(axis, MajorRadius, MinorRadius);
        if (!mc.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(mc.Status()));

        this->myCurve = mc.Value();
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

PyObject *Geom2dEllipse::getPyObject()
{
    return new Ellipse2dPy(static_cast<Geom2dEllipse*>(this->clone()));
}

void Geom2dEllipse::setHandle(const Handle(Geom2d_Ellipse) &e)
{
    this->myCurve = Handle(Geom2d_Ellipse)::DownCast(e->Copy());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dArcOfEllipse, Part::Geom2dArcOfConic)

Geom2dArcOfEllipse::Geom2dArcOfEllipse()
{
    Handle(Geom2d_Ellipse) e = new Geom2d_Ellipse(gp_Elips2d());
    this->myCurve = new Geom2d_TrimmedCurve(e, e->FirstParameter(),e->LastParameter());
}

Geom2dArcOfEllipse::Geom2dArcOfEllipse(const Handle(Geom2d_Ellipse)& e)
{
    this->myCurve = new Geom2d_TrimmedCurve(e, e->FirstParameter(),e->LastParameter());
}

Geom2dArcOfEllipse::~Geom2dArcOfEllipse() = default;

void Geom2dArcOfEllipse::setHandle(const Handle(Geom2d_TrimmedCurve)& c)
{
    Handle(Geom2d_Ellipse) basis = Handle(Geom2d_Ellipse)::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not an ellipse");
    this->myCurve = Handle(Geom2d_TrimmedCurve)::DownCast(c->Copy());
}

const Handle(Geom2d_Geometry)& Geom2dArcOfEllipse::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dArcOfEllipse::clone() const
{
    Geom2dArcOfEllipse* copy = new Geom2dArcOfEllipse();
    copy->setHandle(this->myCurve);
    return copy;
}

double Geom2dArcOfEllipse::getMajorRadius() const
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(myCurve->BasisCurve());
    return ellipse->MajorRadius();
}

void Geom2dArcOfEllipse::setMajorRadius(double Radius)
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(myCurve->BasisCurve());

    try {
        ellipse->SetMajorRadius(Radius);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

double Geom2dArcOfEllipse::getMinorRadius() const
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(myCurve->BasisCurve());
    return ellipse->MinorRadius();
}

void Geom2dArcOfEllipse::setMinorRadius(double Radius)
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(myCurve->BasisCurve());

    try {
        ellipse->SetMinorRadius(Radius);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

/*!
 * \brief Geom2dArcOfEllipse::getMajorAxisDir
 * \return the direction vector (unit-length) of major axis of the ellipse. The
 * direction also points to the first focus.
 */
Base::Vector2d Geom2dArcOfEllipse::getMajorAxisDir() const
{
    Handle(Geom2d_Ellipse) c = Handle(Geom2d_Ellipse)::DownCast(myCurve->BasisCurve());
    assert(!c.IsNull());
    gp_Dir2d xdir = c->XAxis().Direction();
    return {xdir.X(), xdir.Y()};
}

/*!
 * \brief Geom2dArcOfEllipse::setMajorAxisDir Rotates the ellipse in its plane, so
 * that its major axis is as close as possible to the provided direction.
 * \param newdir [in] is the new direction. If the vector is small, the
 * orientation of the ellipse will be preserved. If the vector is not small,
 * but its projection onto plane of the ellipse is small, an exception will be
 * thrown.
 */
void Geom2dArcOfEllipse::setMajorAxisDir(Base::Vector2d newdir)
{
    Handle(Geom2d_Ellipse) c = Handle(Geom2d_Ellipse)::DownCast(myCurve->BasisCurve());
    assert(!c.IsNull());
    if (newdir.Length() < Precision::Confusion())
        return;//zero vector was passed. Keep the old orientation.
    try {
        gp_Elips2d e = c->Elips2d();
        gp_Ax22d pos = e.Axis();
        pos.SetXDirection(gp_Dir2d(newdir.x, newdir.y));
        e.SetAxis(pos);
        c->SetElips2d(e);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

unsigned int Geom2dArcOfEllipse::getMemSize () const
{
    return sizeof(Geom2d_Ellipse) + 2 *sizeof(double);
}

void Geom2dArcOfEllipse::Save(Base::Writer &writer) const
{
    // save the attributes of the father class
    Geom2dCurve::Save(writer);

    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(this->myCurve->BasisCurve());

    gp_Elips2d e = ellipse->Elips2d();
    gp_Ax22d axis = e.Axis();
    double u = this->myCurve->FirstParameter();
    double v = this->myCurve->LastParameter();

    writer.Stream()
        << writer.ind()
        << "<Geom2dArcOfEllipse ";
    SaveAxis(writer, axis, u, v);
    writer.Stream()
        << "MajorRadius=\"" << e.MajorRadius() << "\" "
        << "MinorRadius=\"" << e.MinorRadius() << "\" "
        << "/>" << endl;
}

void Geom2dArcOfEllipse::Restore(Base::XMLReader &reader)
{
    // read the attributes of the father class
    Geom2dCurve::Restore(reader);

    double MajorRadius,MinorRadius,u,v;
    gp_Ax22d axis;
    // read my Element
    reader.readElement("Geom2dArcOfEllipse");
    // get the value of my Attribute
    RestoreAxis(reader, axis, u, v);
    MajorRadius = reader.getAttributeAsFloat("MajorRadius");
    MinorRadius = reader.getAttributeAsFloat("MinorRadius");

    try {
        GCE2d_MakeEllipse mc(axis, MajorRadius, MinorRadius);
        if (!mc.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(mc.Status()));

        GCE2d_MakeArcOfEllipse ma(mc.Value()->Elips2d(), u, v);
        if (!ma.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(ma.Status()));

        Handle(Geom2d_TrimmedCurve) tmpcurve = ma.Value();
        Handle(Geom2d_Ellipse) tmpellipse = Handle(Geom2d_Ellipse)::DownCast(tmpcurve->BasisCurve());
        Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(this->myCurve->BasisCurve());

        ellipse->SetElips2d(tmpellipse->Elips2d());
        this->myCurve->SetTrim(tmpcurve->FirstParameter(), tmpcurve->LastParameter());
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

PyObject *Geom2dArcOfEllipse::getPyObject()
{
    return new ArcOfEllipse2dPy(static_cast<Geom2dArcOfEllipse*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dHyperbola, Part::Geom2dConic)

Geom2dHyperbola::Geom2dHyperbola()
{
    Handle(Geom2d_Hyperbola) h = new Geom2d_Hyperbola(gp_Hypr2d());
    this->myCurve = h;
}

Geom2dHyperbola::Geom2dHyperbola(const Handle(Geom2d_Hyperbola)& h)
{
    this->myCurve = Handle(Geom2d_Hyperbola)::DownCast(h->Copy());
}

Geom2dHyperbola::~Geom2dHyperbola() = default;

const Handle(Geom2d_Geometry)& Geom2dHyperbola::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dHyperbola::clone() const
{
    Geom2dHyperbola *newHyp = new Geom2dHyperbola(myCurve);
    return newHyp;
}

double Geom2dHyperbola::getMajorRadius() const
{
    Handle(Geom2d_Hyperbola) h = Handle(Geom2d_Hyperbola)::DownCast(handle());
    return h->MajorRadius();
}

void Geom2dHyperbola::setMajorRadius(double Radius)
{
    Handle(Geom2d_Hyperbola) h = Handle(Geom2d_Hyperbola)::DownCast(handle());

    try {
        h->SetMajorRadius(Radius);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

double Geom2dHyperbola::getMinorRadius() const
{
    Handle(Geom2d_Hyperbola) h = Handle(Geom2d_Hyperbola)::DownCast(handle());
    return h->MinorRadius();
}

void Geom2dHyperbola::setMinorRadius(double Radius)
{
    Handle(Geom2d_Hyperbola) h = Handle(Geom2d_Hyperbola)::DownCast(handle());

    try {
        h->SetMinorRadius(Radius);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

unsigned int Geom2dHyperbola::getMemSize () const
{
    return sizeof(Geom2d_Hyperbola);
}

void Geom2dHyperbola::Save(Base::Writer& writer) const
{
    // save the attributes of the father class
    Geom2dCurve::Save(writer);

    gp_Hypr2d h = this->myCurve->Hypr2d();
    gp_Ax22d axis = h.Axis();

    writer.Stream()
        << writer.ind()
        << "<Geom2dHyperbola ";
    SaveAxis(writer, axis);
    writer.Stream()
        << "MajorRadius=\"" <<  h.MajorRadius() << "\" "
        << "MinorRadius=\"" <<  h.MinorRadius() << "\" "
        << "/>" << endl;
}

void Geom2dHyperbola::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    Geom2dCurve::Restore(reader);

    double MajorRadius,MinorRadius;
    gp_Ax22d axis;
    // read my Element
    reader.readElement("Geom2dHyperbola");
    // get the value of my Attribute
    RestoreAxis(reader, axis);
    MajorRadius = reader.getAttributeAsFloat("MajorRadius");
    MinorRadius = reader.getAttributeAsFloat("MinorRadius");

    try {
        GCE2d_MakeHyperbola mc(axis, MajorRadius, MinorRadius);
        if (!mc.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(mc.Status()));

        this->myCurve = mc.Value();
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

PyObject *Geom2dHyperbola::getPyObject()
{
    return new Hyperbola2dPy(static_cast<Geom2dHyperbola*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dArcOfHyperbola, Part::Geom2dArcOfConic)

Geom2dArcOfHyperbola::Geom2dArcOfHyperbola()
{
    Handle(Geom2d_Hyperbola) h = new Geom2d_Hyperbola(gp_Hypr2d());
    this->myCurve = new Geom2d_TrimmedCurve(h, h->FirstParameter(),h->LastParameter());
}

Geom2dArcOfHyperbola::Geom2dArcOfHyperbola(const Handle(Geom2d_Hyperbola)& h)
{
    this->myCurve = new Geom2d_TrimmedCurve(h, h->FirstParameter(),h->LastParameter());
}

Geom2dArcOfHyperbola::~Geom2dArcOfHyperbola() = default;

void Geom2dArcOfHyperbola::setHandle(const Handle(Geom2d_TrimmedCurve)& c)
{
    Handle(Geom2d_Hyperbola) basis = Handle(Geom2d_Hyperbola)::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not an hyperbola");
    this->myCurve = Handle(Geom2d_TrimmedCurve)::DownCast(c->Copy());
}

const Handle(Geom2d_Geometry)& Geom2dArcOfHyperbola::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dArcOfHyperbola::clone() const
{
    Geom2dArcOfHyperbola* copy = new Geom2dArcOfHyperbola();
    copy->setHandle(this->myCurve);
    return copy;
}

double Geom2dArcOfHyperbola::getMajorRadius() const
{
    Handle(Geom2d_Hyperbola) h = Handle(Geom2d_Hyperbola)::DownCast(myCurve->BasisCurve());
    return h->MajorRadius();
}

void Geom2dArcOfHyperbola::setMajorRadius(double Radius)
{
    Handle(Geom2d_Hyperbola) h = Handle(Geom2d_Hyperbola)::DownCast(myCurve->BasisCurve());

    try {
        h->SetMajorRadius(Radius);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

double Geom2dArcOfHyperbola::getMinorRadius() const
{
    Handle(Geom2d_Hyperbola) h = Handle(Geom2d_Hyperbola)::DownCast(myCurve->BasisCurve());
    return h->MinorRadius();
}

void Geom2dArcOfHyperbola::setMinorRadius(double Radius)
{
    Handle(Geom2d_Hyperbola) h = Handle(Geom2d_Hyperbola)::DownCast(myCurve->BasisCurve());

    try {
        h->SetMinorRadius(Radius);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

unsigned int Geom2dArcOfHyperbola::getMemSize () const
{
    return sizeof(Geom2d_Hyperbola) + 2 *sizeof(double);
}

void Geom2dArcOfHyperbola::Save(Base::Writer &writer) const
{
    // save the attributes of the father class
    Geom2dCurve::Save(writer);

    Handle(Geom2d_Hyperbola) hh = Handle(Geom2d_Hyperbola)::DownCast(this->myCurve->BasisCurve());

    gp_Hypr2d h = hh->Hypr2d();
    gp_Ax22d axis = h.Axis();
    double u = this->myCurve->FirstParameter();
    double v = this->myCurve->LastParameter();

    writer.Stream()
        << writer.ind()
        << "<Geom2dHyperbola ";
    SaveAxis(writer, axis, u, v);
    writer.Stream()
        << "MajorRadius=\"" <<  h.MajorRadius() << "\" "
        << "MinorRadius=\"" <<  h.MinorRadius() << "\" "
        << "/>" << endl;
}

void Geom2dArcOfHyperbola::Restore(Base::XMLReader &reader)
{
    // read the attributes of the father class
    Geom2dCurve::Restore(reader);

    double MajorRadius,MinorRadius,u,v;
    gp_Ax22d axis;
    // read my Element
    reader.readElement("Geom2dHyperbola");
    // get the value of my Attribute
    RestoreAxis(reader, axis, u, v);
    MajorRadius = reader.getAttributeAsFloat("MajorRadius");
    MinorRadius = reader.getAttributeAsFloat("MinorRadius");

    try {
        GCE2d_MakeHyperbola mc(axis, MajorRadius, MinorRadius);
        if (!mc.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(mc.Status()));

        GCE2d_MakeArcOfHyperbola ma(mc.Value()->Hypr2d(), u, v);
        if (!ma.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(ma.Status()));

        Handle(Geom2d_TrimmedCurve) tmpcurve = ma.Value();
        Handle(Geom2d_Hyperbola) tmphyperbola = Handle(Geom2d_Hyperbola)::DownCast(tmpcurve->BasisCurve());
        Handle(Geom2d_Hyperbola) hyperbola = Handle(Geom2d_Hyperbola)::DownCast(this->myCurve->BasisCurve());

        hyperbola->SetHypr2d(tmphyperbola->Hypr2d());
        this->myCurve->SetTrim(tmpcurve->FirstParameter(), tmpcurve->LastParameter());
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

PyObject *Geom2dArcOfHyperbola::getPyObject()
{
    return new ArcOfHyperbola2dPy(static_cast<Geom2dArcOfHyperbola*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dParabola, Part::Geom2dConic)

Geom2dParabola::Geom2dParabola()
{
    Handle(Geom2d_Parabola) p = new Geom2d_Parabola(gp_Parab2d());
    this->myCurve = p;
}

Geom2dParabola::Geom2dParabola(const Handle(Geom2d_Parabola)& p)
{
    this->myCurve = Handle(Geom2d_Parabola)::DownCast(p->Copy());
}

Geom2dParabola::~Geom2dParabola() = default;

const Handle(Geom2d_Geometry)& Geom2dParabola::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dParabola::clone() const
{
    Geom2dParabola *newPar = new Geom2dParabola(myCurve);
    return newPar;
}

double Geom2dParabola::getFocal() const
{
    Handle(Geom2d_Parabola) p = Handle(Geom2d_Parabola)::DownCast(handle());
    return p->Focal();
}

void Geom2dParabola::setFocal(double length)
{
    Handle(Geom2d_Parabola) p = Handle(Geom2d_Parabola)::DownCast(handle());

    try {
        p->SetFocal(length);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

unsigned int Geom2dParabola::getMemSize () const
{
    return sizeof(Geom2d_Parabola);
}

void Geom2dParabola::Save(Base::Writer& writer) const
{
    // save the attributes of the father class
    Geom2dCurve::Save(writer);

    gp_Parab2d p = this->myCurve->Parab2d();
    gp_Ax22d axis = p.Axis();
    double focal = p.Focal();

    writer.Stream()
        << writer.ind()
        << "<Geom2dParabola ";
    SaveAxis(writer, axis);
    writer.Stream()
        << "Focal=\"" << focal << "\" "
        << "/>" << endl;
}

void Geom2dParabola::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    Geom2dCurve::Restore(reader);

    double Focal;
    // read my Element
    reader.readElement("Geom2dParabola");
    gp_Ax22d axis;
    // get the value of my Attribute
    RestoreAxis(reader, axis);
    Focal = reader.getAttributeAsFloat("Focal");

    try {
        GCE2d_MakeParabola mc(axis, Focal);
        if (!mc.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(mc.Status()));

        this->myCurve = mc.Value();
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

PyObject *Geom2dParabola::getPyObject()
{
    return new Parabola2dPy(static_cast<Geom2dParabola*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dArcOfParabola, Part::Geom2dArcOfConic)

Geom2dArcOfParabola::Geom2dArcOfParabola()
{
    Handle(Geom2d_Parabola) p = new Geom2d_Parabola(gp_Parab2d());
    this->myCurve = new Geom2d_TrimmedCurve(p, p->FirstParameter(),p->LastParameter());
}

Geom2dArcOfParabola::Geom2dArcOfParabola(const Handle(Geom2d_Parabola)& h)
{
    this->myCurve = new Geom2d_TrimmedCurve(h, h->FirstParameter(),h->LastParameter());
}

Geom2dArcOfParabola::~Geom2dArcOfParabola() = default;

void Geom2dArcOfParabola::setHandle(const Handle(Geom2d_TrimmedCurve)& c)
{
    Handle(Geom2d_Parabola) basis = Handle(Geom2d_Parabola)::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not a parabola");
    this->myCurve = Handle(Geom2d_TrimmedCurve)::DownCast(c->Copy());
}

const Handle(Geom2d_Geometry)& Geom2dArcOfParabola::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dArcOfParabola::clone() const
{
    Geom2dArcOfParabola* copy = new Geom2dArcOfParabola();
    copy->setHandle(this->myCurve);
    return copy;
}

double Geom2dArcOfParabola::getFocal() const
{
    Handle(Geom2d_Parabola) p = Handle(Geom2d_Parabola)::DownCast(myCurve->BasisCurve());
    return p->Focal();
}

void Geom2dArcOfParabola::setFocal(double length)
{
    Handle(Geom2d_Parabola) p = Handle(Geom2d_Parabola)::DownCast(myCurve->BasisCurve());

    try {
        p->SetFocal(length);
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

unsigned int Geom2dArcOfParabola::getMemSize () const
{
    return sizeof(Geom2d_Parabola) + 2 *sizeof(double);
}

void Geom2dArcOfParabola::Save(Base::Writer &writer) const
{
    // save the attributes of the father class
    Geom2dCurve::Save(writer);

    Handle(Geom2d_Parabola) hp = Handle(Geom2d_Parabola)::DownCast(this->myCurve->BasisCurve());
    gp_Parab2d p = hp->Parab2d();
    gp_Ax22d axis = p.Axis();
    double u = this->myCurve->FirstParameter();
    double v = this->myCurve->LastParameter();
    double focal = p.Focal();

    writer.Stream()
        << writer.ind()
        << "<Geom2dArcOfParabola ";
    SaveAxis(writer, axis, u, v);
    writer.Stream()
        << "Focal=\"" << focal << "\" "
        << "/>" << endl;
}

void Geom2dArcOfParabola::Restore(Base::XMLReader &reader)
{
    // read the attributes of the father class
    Geom2dCurve::Restore(reader);

    double Focal,u,v;
    gp_Ax22d axis;
    // read my Element
    reader.readElement("Geom2dParabola");
    // get the value of my Attribute
    RestoreAxis(reader, axis, u, v);
    Focal = reader.getAttributeAsFloat("Focal");

    try {
        GCE2d_MakeParabola mc(axis, Focal);
        if (!mc.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(mc.Status()));

        GCE2d_MakeArcOfParabola ma(mc.Value()->Parab2d(), u, v);
        if (!ma.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(ma.Status()));

        Handle(Geom2d_TrimmedCurve) tmpcurve = ma.Value();
        Handle(Geom2d_Parabola) tmpparabola = Handle(Geom2d_Parabola)::DownCast(tmpcurve->BasisCurve());
        Handle(Geom2d_Parabola) parabola = Handle(Geom2d_Parabola)::DownCast(this->myCurve->BasisCurve());

        parabola->SetParab2d(tmpparabola->Parab2d());
        this->myCurve->SetTrim(tmpcurve->FirstParameter(), tmpcurve->LastParameter());
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

PyObject *Geom2dArcOfParabola::getPyObject()
{
    return new ArcOfParabola2dPy(static_cast<Geom2dArcOfParabola*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dLine, Part::Geom2dCurve)

Geom2dLine::Geom2dLine()
{
    Handle(Geom2d_Line) c = new Geom2d_Line(gp_Lin2d());
    this->myCurve = c;
}

Geom2dLine::Geom2dLine(const Handle(Geom2d_Line)& l)
{
    this->myCurve = Handle(Geom2d_Line)::DownCast(l->Copy());
}

Geom2dLine::Geom2dLine(const Base::Vector2d& Pos, const Base::Vector2d& Dir)
{
    this->myCurve = new Geom2d_Line(gp_Pnt2d(Pos.x,Pos.y),gp_Dir2d(Dir.x,Dir.y));
}

Geom2dLine::~Geom2dLine() = default;

void Geom2dLine::setLine(const Base::Vector2d& Pos, const Base::Vector2d& Dir)
{
    this->myCurve->SetLocation(gp_Pnt2d(Pos.x,Pos.y));
    this->myCurve->SetDirection(gp_Dir2d(Dir.x,Dir.y));
}

Base::Vector2d Geom2dLine::getPos() const
{
    gp_Pnt2d Pos = this->myCurve->Lin2d().Location();
    return {Pos.X(),Pos.Y()};
}

Base::Vector2d Geom2dLine::getDir() const
{
    gp_Dir2d Dir = this->myCurve->Lin2d().Direction();
    return {Dir.X(),Dir.Y()};
}

const Handle(Geom2d_Geometry)& Geom2dLine::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dLine::clone() const
{
    Geom2dLine *newLine = new Geom2dLine(myCurve);
    return newLine;
}

unsigned int Geom2dLine::getMemSize () const
{
    return sizeof(Geom2d_Line);
}

void Geom2dLine::Save(Base::Writer &writer) const
{
    // save the attributes of the father class
    Geometry2d::Save(writer);

    Base::Vector2d Pos = getPos();
    Base::Vector2d Dir = getDir();

    writer.Stream()
        << writer.ind()
        << "<Geom2dLine "
        << "PosX=\"" << Pos.x << "\" "
        << "PosY=\"" << Pos.y << "\" "
        << "DirX=\"" << Dir.x << "\" "
        << "DirY=\"" << Dir.y << "\" "
        << "/>" << endl;
}

void Geom2dLine::Restore(Base::XMLReader &reader)
{
    // read the attributes of the father class
    Geom2dCurve::Restore(reader);

    double PosX,PosY,DirX,DirY;
    // read my Element
    reader.readElement("Geom2dLine");
    // get the value of my Attribute
    PosX = reader.getAttributeAsFloat("PosX");
    PosY = reader.getAttributeAsFloat("PosY");
    DirX = reader.getAttributeAsFloat("DirX");
    DirY = reader.getAttributeAsFloat("DirY");
    gp_Pnt2d pnt(PosX, PosY);
    gp_Dir2d dir(DirX, DirY);

    try {
        GCE2d_MakeLine mc(pnt, dir);
        if (!mc.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(mc.Status()));

        this->myCurve = mc.Value();
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

PyObject *Geom2dLine::getPyObject()
{
    return new Line2dPy(static_cast<Geom2dLine*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dLineSegment, Part::Geom2dCurve)

Geom2dLineSegment::Geom2dLineSegment()
{
    gp_Lin2d line;
    line.SetLocation(gp_Pnt2d(0.0,0.0));
    line.SetDirection(gp_Dir2d(0.0,1.0));
    Handle(Geom2d_Line) c = new Geom2d_Line(line);
    this->myCurve = new Geom2d_TrimmedCurve(c, 0.0,1.0);
}

Geom2dLineSegment::~Geom2dLineSegment() = default;

void Geom2dLineSegment::setHandle(const Handle(Geom2d_TrimmedCurve)& c)
{
    Handle(Geom2d_Line) basis = Handle(Geom2d_Line)::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not a line");
    this->myCurve = Handle(Geom2d_TrimmedCurve)::DownCast(c->Copy());
}

const Handle(Geom2d_Geometry)& Geom2dLineSegment::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dLineSegment::clone()const
{
    Geom2dLineSegment *tempCurve = new Geom2dLineSegment();
    tempCurve->myCurve = Handle(Geom2d_TrimmedCurve)::DownCast(myCurve->Copy());
    return tempCurve;
}

Base::Vector2d Geom2dLineSegment::getStartPoint() const
{
    Handle(Geom2d_TrimmedCurve) this_curve = Handle(Geom2d_TrimmedCurve)::DownCast(handle());
    gp_Pnt2d pnt = this_curve->StartPoint();
    return {pnt.X(), pnt.Y()};
}

Base::Vector2d Geom2dLineSegment::getEndPoint() const
{
    Handle(Geom2d_TrimmedCurve) this_curve = Handle(Geom2d_TrimmedCurve)::DownCast(handle());
    gp_Pnt2d pnt = this_curve->EndPoint();
    return {pnt.X(), pnt.Y()};
}

void Geom2dLineSegment::setPoints(const Base::Vector2d& Start, const Base::Vector2d& End)
{
    gp_Pnt2d p1(Start.x,Start.y), p2(End.x,End.y);
    Handle(Geom2d_TrimmedCurve) this_curv = Handle(Geom2d_TrimmedCurve)::DownCast(handle());

    try {
        // Create line out of two points
        if (p1.Distance(p2) < gp::Resolution())
            Standard_Failure::Raise("Both points are equal");
        GCE2d_MakeSegment ms(p1, p2);
        if (!ms.IsDone()) {
            throw Base::CADKernelError(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line segment
        Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
            (this_curv->BasisCurve());
        Handle(Geom2d_TrimmedCurve) that_curv = ms.Value();
        Handle(Geom2d_Line) that_line = Handle(Geom2d_Line)::DownCast(that_curv->BasisCurve());
        this_line->SetLin2d(that_line->Lin2d());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

unsigned int Geom2dLineSegment::getMemSize () const
{
    return sizeof(Geom2d_TrimmedCurve) + sizeof(Geom2d_Line);
}

void Geom2dLineSegment::Save(Base::Writer &writer) const
{
    // save the attributes of the father class
    Geom2dCurve::Save(writer);

    Base::Vector2d End   =  getEndPoint();
    Base::Vector2d Start =  getStartPoint();

    writer.Stream()
        << writer.ind()
        << "<Geom2dLineSegment "
        << "StartX=\"" << Start.x << "\" "
        << "StartY=\"" << Start.y << "\" "
        << "EndX=\"" << End.x << "\" "
        << "EndY=\"" << End.y << "\" "
        << "/>" << endl;
}

void Geom2dLineSegment::Restore(Base::XMLReader &reader)
{
    // read the attributes of the father class
    Geom2dCurve::Restore(reader);

    double StartX,StartY,EndX,EndY;
    // read my Element
    reader.readElement("Geom2dLineSegment");
    // get the value of my Attribute
    StartX = reader.getAttributeAsFloat("StartX");
    StartY = reader.getAttributeAsFloat("StartY");
    EndX   = reader.getAttributeAsFloat("EndX");
    EndY   = reader.getAttributeAsFloat("EndY");

    gp_Pnt2d p1(StartX, StartY);
    gp_Pnt2d p2(EndX, EndY);

    try {
        GCE2d_MakeSegment mc(p1, p2);
        if (!mc.IsDone())
            throw Base::CADKernelError(gce_ErrorStatusText(mc.Status()));

        this->myCurve = mc.Value();
    }
    catch (Standard_Failure& e) {
        throw Base::CADKernelError(e.GetMessageString());
    }
}

PyObject *Geom2dLineSegment::getPyObject()
{
    return new Line2dSegmentPy(static_cast<Geom2dLineSegment*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dOffsetCurve, Part::Geom2dCurve)

Geom2dOffsetCurve::Geom2dOffsetCurve() = default;

Geom2dOffsetCurve::Geom2dOffsetCurve(const Handle(Geom2d_Curve)& c, double offset)
{
    this->myCurve = new Geom2d_OffsetCurve(c, offset);
}

Geom2dOffsetCurve::Geom2dOffsetCurve(const Handle(Geom2d_OffsetCurve)& c)
{
    this->myCurve = Handle(Geom2d_OffsetCurve)::DownCast(c->Copy());
}

Geom2dOffsetCurve::~Geom2dOffsetCurve() = default;

Geometry2d *Geom2dOffsetCurve::clone() const
{
    Geom2dOffsetCurve *newCurve = new Geom2dOffsetCurve(myCurve);
    return newCurve;
}

void Geom2dOffsetCurve::setHandle(const Handle(Geom2d_OffsetCurve)& c)
{
    this->myCurve = Handle(Geom2d_OffsetCurve)::DownCast(c->Copy());
}

const Handle(Geom2d_Geometry)& Geom2dOffsetCurve::handle() const
{
    return this->myCurve;
}

unsigned int Geom2dOffsetCurve::getMemSize () const
{
    throw Base::NotImplementedError("Geom2dOffsetCurve::getMemSize");
}

void Geom2dOffsetCurve::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("Geom2dOffsetCurve::Save");
}

void Geom2dOffsetCurve::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("Geom2dOffsetCurve::Restore");
}

PyObject *Geom2dOffsetCurve::getPyObject()
{
    return new OffsetCurve2dPy(static_cast<Geom2dOffsetCurve*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::Geom2dTrimmedCurve, Part::Geom2dCurve)

Geom2dTrimmedCurve::Geom2dTrimmedCurve() = default;

Geom2dTrimmedCurve::Geom2dTrimmedCurve(const Handle(Geom2d_TrimmedCurve)& c)
{
    this->myCurve = Handle(Geom2d_TrimmedCurve)::DownCast(c->Copy());
}

Geom2dTrimmedCurve::~Geom2dTrimmedCurve() = default;

void Geom2dTrimmedCurve::setHandle(const Handle(Geom2d_TrimmedCurve)& c)
{
    this->myCurve = Handle(Geom2d_TrimmedCurve)::DownCast(c->Copy());
}

const Handle(Geom2d_Geometry)& Geom2dTrimmedCurve::handle() const
{
    return myCurve;
}

Geometry2d *Geom2dTrimmedCurve::clone() const
{
    Geom2dTrimmedCurve *newCurve =  new Geom2dTrimmedCurve(myCurve);
    return newCurve;
}

unsigned int Geom2dTrimmedCurve::getMemSize () const
{
    throw Base::NotImplementedError("Geom2dTrimmedCurve::getMemSize");
}

void Geom2dTrimmedCurve::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("Geom2dTrimmedCurve::Save");
}

void Geom2dTrimmedCurve::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("Geom2dTrimmedCurve::Restore");
}

PyObject *Geom2dTrimmedCurve::getPyObject()
{
    Handle(Geom2d_Curve) basis = this->myCurve->BasisCurve();
    if (basis.IsNull())
        Py_Return;
    if (basis->IsKind(STANDARD_TYPE (Geom2d_Parabola))) {
        Geom2dArcOfParabola c;
        c.setHandle(this->myCurve);
        return c.getPyObject();
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_Hyperbola))) {
        Geom2dArcOfHyperbola c;
        c.setHandle(this->myCurve);
        return c.getPyObject();
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_Ellipse))) {
        Geom2dArcOfEllipse c;
        c.setHandle(this->myCurve);
        return c.getPyObject();
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_Circle))) {
        Geom2dArcOfCircle c;
        c.setHandle(this->myCurve);
        return c.getPyObject();
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_Line))) {
        Geom2dLineSegment c;
        c.setHandle(this->myCurve);
        return c.getPyObject();
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_BSplineCurve))) {
        Geom2dBSplineCurve c;
        c.setHandle(Handle(Geom2d_BSplineCurve)::DownCast(basis));
        return c.getPyObject();
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_BezierCurve))) {
        Geom2dBezierCurve c;
        c.setHandle(Handle(Geom2d_BezierCurve)::DownCast(basis));
        return c.getPyObject();
    }

    PyErr_SetString(PyExc_RuntimeError, "Unknown curve type");
    return nullptr;
}

// ------------------------------------------------------------------

namespace Part {
std::unique_ptr<Geom2dCurve> makeFromCurve2d(Handle(Geom2d_Curve) curve)
{
    std::unique_ptr<Geom2dCurve> geo2d;
    if (curve.IsNull())
        return geo2d;
    if (curve->IsKind(STANDARD_TYPE (Geom2d_Parabola))) {
        geo2d = std::make_unique<Geom2dParabola>(Handle(Geom2d_Parabola)::DownCast(curve));
    }
    else if (curve->IsKind(STANDARD_TYPE (Geom2d_Hyperbola))) {
        geo2d = std::make_unique<Geom2dHyperbola>(Handle(Geom2d_Hyperbola)::DownCast(curve));
    }
    else if (curve->IsKind(STANDARD_TYPE (Geom2d_Ellipse))) {
        geo2d = std::make_unique<Geom2dEllipse>(Handle(Geom2d_Ellipse)::DownCast(curve));
    }
    else if (curve->IsKind(STANDARD_TYPE (Geom2d_Circle))) {
        geo2d = std::make_unique<Geom2dCircle>(Handle(Geom2d_Circle)::DownCast(curve));
    }
    else if (curve->IsKind(STANDARD_TYPE (Geom2d_Line))) {
        geo2d = std::make_unique<Geom2dLine>(Handle(Geom2d_Line)::DownCast(curve));
    }
    else if (curve->IsKind(STANDARD_TYPE (Geom2d_BSplineCurve))) {
        geo2d = std::make_unique<Geom2dBSplineCurve>(Handle(Geom2d_BSplineCurve)::DownCast(curve));
    }
    else if (curve->IsKind(STANDARD_TYPE (Geom2d_BezierCurve))) {
        geo2d = std::make_unique<Geom2dBezierCurve>(Handle(Geom2d_BezierCurve)::DownCast(curve));
    }
    else if (curve->IsKind(STANDARD_TYPE (Geom2d_TrimmedCurve))) {
        geo2d = std::make_unique<Geom2dTrimmedCurve>(Handle(Geom2d_TrimmedCurve)::DownCast(curve));
    }

    return geo2d;
}

std::unique_ptr<Geom2dCurve> makeFromTrimmedCurve2d(const Handle(Geom2d_Curve)& c, double f, double l)
{
    if (c->IsKind(STANDARD_TYPE(Geom2d_Circle))) {
        Handle(Geom2d_Circle) circ = Handle(Geom2d_Circle)::DownCast(c);
        std::unique_ptr<Geom2dCurve> arc(new Geom2dArcOfCircle());

        Handle(Geom2d_TrimmedCurve) this_arc = Handle(Geom2d_TrimmedCurve)::DownCast
            (arc->handle());
        Handle(Geom2d_Circle) this_circ = Handle(Geom2d_Circle)::DownCast
            (this_arc->BasisCurve());
        this_circ->SetCirc2d(circ->Circ2d());
        this_arc->SetTrim(f, l);
        return arc;
    }
    else if (c->IsKind(STANDARD_TYPE(Geom2d_Ellipse))) {
        Handle(Geom2d_Ellipse) ellp = Handle(Geom2d_Ellipse)::DownCast(c);
        std::unique_ptr<Geom2dCurve> arc(new Geom2dArcOfEllipse());

        Handle(Geom2d_TrimmedCurve) this_arc = Handle(Geom2d_TrimmedCurve)::DownCast
            (arc->handle());
        Handle(Geom2d_Ellipse) this_ellp = Handle(Geom2d_Ellipse)::DownCast
            (this_arc->BasisCurve());
        this_ellp->SetElips2d(ellp->Elips2d());
        this_arc->SetTrim(f, l);
        return arc;
    }
    else if (c->IsKind(STANDARD_TYPE(Geom2d_Hyperbola))) {
        Handle(Geom2d_Hyperbola) hypr = Handle(Geom2d_Hyperbola)::DownCast(c);
        std::unique_ptr<Geom2dCurve> arc(new Geom2dArcOfHyperbola());

        Handle(Geom2d_TrimmedCurve) this_arc = Handle(Geom2d_TrimmedCurve)::DownCast
            (arc->handle());
        Handle(Geom2d_Hyperbola) this_hypr = Handle(Geom2d_Hyperbola)::DownCast
            (this_arc->BasisCurve());
        this_hypr->SetHypr2d(hypr->Hypr2d());
        this_arc->SetTrim(f, l);
        return arc;
    }
    else if (c->IsKind(STANDARD_TYPE(Geom2d_Line))) {
        Handle(Geom2d_Line) line = Handle(Geom2d_Line)::DownCast(c);
        std::unique_ptr<Geom2dCurve> segm(new Geom2dLineSegment());

        Handle(Geom2d_TrimmedCurve) this_segm = Handle(Geom2d_TrimmedCurve)::DownCast
            (segm->handle());
        Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
            (this_segm->BasisCurve());
        this_line->SetLin2d(line->Lin2d());
        this_segm->SetTrim(f, l);
        return segm;
    }
    else if (c->IsKind(STANDARD_TYPE(Geom2d_Parabola))) {
        Handle(Geom2d_Parabola) para = Handle(Geom2d_Parabola)::DownCast(c);
        std::unique_ptr<Geom2dCurve> arc(new Geom2dArcOfParabola());

        Handle(Geom2d_TrimmedCurve) this_arc = Handle(Geom2d_TrimmedCurve)::DownCast
            (arc->handle());
        Handle(Geom2d_Parabola) this_para = Handle(Geom2d_Parabola)::DownCast
            (this_arc->BasisCurve());
        this_para->SetParab2d(para->Parab2d());
        this_arc->SetTrim(f, l);
        return arc;
    }
    else if (c->IsKind(STANDARD_TYPE(Geom2d_BezierCurve))) {
        Handle(Geom2d_BezierCurve) bezier = Handle(Geom2d_BezierCurve)::DownCast(c->Copy());
        bezier->Segment(f, l);
        return std::unique_ptr<Geom2dCurve>(new Geom2dBezierCurve(bezier));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom2d_BSplineCurve))) {
        Handle(Geom2d_BSplineCurve) bspline = Handle(Geom2d_BSplineCurve)::DownCast(c->Copy());
        bspline->Segment(f, l);
        return std::unique_ptr<Geom2dCurve>(new Geom2dBSplineCurve(bspline));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom2d_OffsetCurve))) {
        Handle(Geom2d_OffsetCurve) oc = Handle(Geom2d_OffsetCurve)::DownCast(c);
        double v = oc->Offset();
        std::unique_ptr<Geom2dCurve> bc(makeFromTrimmedCurve2d(oc->BasisCurve(), f, l));
        return std::unique_ptr<Geom2dCurve>(new Geom2dOffsetCurve(Handle(Geom2d_Curve)::DownCast(bc->handle()), v));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom2d_TrimmedCurve))) {
        Handle(Geom2d_TrimmedCurve) trc = Handle(Geom2d_TrimmedCurve)::DownCast(c);
        return makeFromTrimmedCurve2d(trc->BasisCurve(), f, l);
    }
    else {
        std::string err = "Unhandled curve type ";
        err += c->DynamicType()->Name();
        throw Base::TypeError(err);
    }
}
std::unique_ptr<Geom2dCurve> makeFromCurveAdaptor2d(const Adaptor2d_Curve2d& adapt)
{
    std::unique_ptr<Geom2dCurve> geoCurve;
    switch (adapt.GetType())
    {
    case GeomAbs_Line:
        {
            geoCurve = std::make_unique<Geom2dLine>();
            Handle(Geom2d_Line) this_curv = Handle(Geom2d_Line)::DownCast
                (geoCurve->handle());
            this_curv->SetLin2d(adapt.Line());
            break;
        }
    case GeomAbs_Circle:
        {
            geoCurve = std::make_unique<Geom2dCircle>();
            Handle(Geom2d_Circle) this_curv = Handle(Geom2d_Circle)::DownCast
                (geoCurve->handle());
            this_curv->SetCirc2d(adapt.Circle());
            break;
        }
    case GeomAbs_Ellipse:
        {
            geoCurve = std::make_unique<Geom2dEllipse>();
            Handle(Geom2d_Ellipse) this_curv = Handle(Geom2d_Ellipse)::DownCast
                (geoCurve->handle());
            this_curv->SetElips2d(adapt.Ellipse());
            break;
        }
    case GeomAbs_Hyperbola:
        {
            geoCurve = std::make_unique<Geom2dHyperbola>();
            Handle(Geom2d_Hyperbola) this_curv = Handle(Geom2d_Hyperbola)::DownCast
                (geoCurve->handle());
            this_curv->SetHypr2d(adapt.Hyperbola());
            break;
        }
    case GeomAbs_Parabola:
        {
            geoCurve = std::make_unique<Geom2dParabola>();
            Handle(Geom2d_Parabola) this_curv = Handle(Geom2d_Parabola)::DownCast
                (geoCurve->handle());
            this_curv->SetParab2d(adapt.Parabola());
            break;
        }
    case GeomAbs_BezierCurve:
        {
            geoCurve = std::make_unique<Geom2dBezierCurve>(adapt.Bezier());
            break;
        }
    case GeomAbs_BSplineCurve:
        {
            geoCurve = std::make_unique<Geom2dBSplineCurve>(adapt.BSpline());
            break;
        }
    case GeomAbs_OtherCurve:
    default:
        break;
    }

    if (!geoCurve)
        throw Base::TypeError("Unhandled curve type");

    // Check if the curve must be trimmed
    Handle(Geom2d_Curve) curv2d = Handle(Geom2d_Curve)::DownCast
        (geoCurve->handle());
    double u = curv2d->FirstParameter();
    double v = curv2d->LastParameter();
    if (u != adapt.FirstParameter() || v != adapt.LastParameter()) {
        geoCurve = makeFromTrimmedCurve2d(curv2d, adapt.FirstParameter(), adapt.LastParameter());
    }

    return geoCurve;
}
}
