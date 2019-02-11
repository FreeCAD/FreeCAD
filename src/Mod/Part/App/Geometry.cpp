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
# include <GeomPlate_Surface.hxx>
# include <Geom_RectangularTrimmedSurface.hxx>
# include <Geom_SurfaceOfRevolution.hxx>
# include <Geom_SurfaceOfLinearExtrusion.hxx>
# include <GeomAPI_Interpolate.hxx>
# include <GeomConvert.hxx>
# include <GeomConvert_CompCurveToBSplineCurve.hxx>
# include <GeomLProp_CLProps.hxx>
# include <GeomLProp_SLProps.hxx>
# include <gp.hxx>
# include <gp_Ax2.hxx>
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
# include <Standard_ConstructionError.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TColgp_Array2OfPnt.hxx>
# include <TColgp_Array1OfVec.hxx>
# include <TColgp_HArray1OfPnt.hxx>
# include <TColStd_HArray1OfBoolean.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <gp.hxx>
# include <gp_Lin.hxx>
# include <Geom_Line.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <GC_MakeArcOfCircle.hxx>
# include <GC_MakeCircle.hxx>
# include <GC_MakeArcOfEllipse.hxx>
# include <GC_MakeEllipse.hxx>
# include <gce_MakeParab.hxx>
# include <GC_MakeArcOfParabola.hxx>
# include <GC_MakeHyperbola.hxx>
# include <GC_MakeArcOfHyperbola.hxx>
# include <GC_MakeLine.hxx>
# include <GC_MakeSegment.hxx>
# include <GCPnts_AbscissaPoint.hxx>
# include <Precision.hxx>
# include <GeomAPI_ProjectPointOnCurve.hxx>
# include <GeomAPI_ExtremaCurveCurve.hxx>
# include <ShapeConstruct_Curve.hxx>
# include <LProp_NotDefined.hxx>

# include <ctime>
# include <cmath>
#endif //_PreComp_

#include <Base/VectorPy.h>
#include <Mod/Part/App/LinePy.h>
#include <Mod/Part/App/LineSegmentPy.h>
#include <Mod/Part/App/CirclePy.h>
#include <Mod/Part/App/EllipsePy.h>
#include <Mod/Part/App/ArcPy.h>
#include <Mod/Part/App/ArcOfCirclePy.h>
#include <Mod/Part/App/ArcOfEllipsePy.h>
#include <Mod/Part/App/ArcOfParabolaPy.h>
#include <Mod/Part/App/BezierCurvePy.h>
#include <Mod/Part/App/BSplineCurvePy.h>
#include <Mod/Part/App/HyperbolaPy.h>
#include <Mod/Part/App/ArcOfHyperbolaPy.h>
#include <Mod/Part/App/OffsetCurvePy.h>
#include <Mod/Part/App/ParabolaPy.h>
#include <Mod/Part/App/BezierSurfacePy.h>
#include <Mod/Part/App/BSplineSurfacePy.h>
#include <Mod/Part/App/ConePy.h>
#include <Mod/Part/App/CylinderPy.h>
#include <Mod/Part/App/OffsetSurfacePy.h>
#include <Mod/Part/App/PointPy.h>
#include <Mod/Part/App/PlateSurfacePy.h>
#include <Mod/Part/App/PlanePy.h>
#include <Mod/Part/App/RectangularTrimmedSurfacePy.h>
#include <Mod/Part/App/SpherePy.h>
#include <Mod/Part/App/SurfaceOfExtrusionPy.h>
#include <Mod/Part/App/SurfaceOfRevolutionPy.h>
#include <Mod/Part/App/ToroidPy.h>

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

TYPESYSTEM_SOURCE_ABSTRACT(Part::Geometry,Base::Persistence)

Geometry::Geometry()
  : Construction(false)
{
    createNewTag();
}

Geometry::~Geometry()
{

}

// Persistence implementer
unsigned int Geometry::getMemSize (void) const
{
    return 1;
}

void Geometry::Save(Base::Writer &writer) const
{
    if( extensions.size()>0 ) {

        writer.incInd();

        writer.Stream() << writer.ind() << "<GeoExtensions count=\"" << extensions.size() << "\">" << endl;

        for(auto att:extensions) {
            att->Save(writer);
        }

        writer.decInd();
        writer.Stream() << writer.ind() << "</GeoExtensions>" << endl;
    }

    const char c = Construction?'1':'0';
    writer.Stream() << writer.ind() << "<Construction value=\"" <<  c << "\"/>" << endl;
}

void Geometry::Restore(Base::XMLReader &reader)
{
    reader.readElement();

    if(strcmp(reader.localName(),"GeoExtensions") == 0) {

        int count = reader.getAttributeAsInteger("count");

        for (int i = 0; i < count; i++) {
            reader.readElement("GeoExtension");
            const char* TypeName = reader.getAttribute("type");
            Base::Type type = Base::Type::fromName(TypeName);
            GeometryExtension *newE = (GeometryExtension *)type.createInstance();
            newE->Restore(reader);

            extensions.push_back(std::shared_ptr<GeometryExtension>(newE));
        }

        reader.readEndElement("GeoExtensions");

        reader.readElement("Construction"); // prepare for reading construction attribute
    }
    else if(strcmp(reader.localName(),"Construction") != 0) { // ignore anything not known
        reader.readElement("Construction");
    }

    Construction = (int)reader.getAttributeAsInteger("value")==0?false:true;

}

boost::uuids::uuid Geometry::getTag() const
{
    return tag;
}

const std::vector<std::weak_ptr<GeometryExtension>> Geometry::getExtensions() const
{
    std::vector<std::weak_ptr<GeometryExtension>> wp;

    for(auto & ext:extensions)
        wp.push_back(ext);

    return wp;
}

bool Geometry::hasExtension(Base::Type type) const
{
    for( auto ext : extensions) {
        if(ext->getTypeId() == type)
            return true;
    }

    return false;
}

bool Geometry::hasExtension(std::string name) const
{
    for( auto ext : extensions) {
        if(ext->getName() == name)
            return true;
    }

    return false;
}

const std::weak_ptr<GeometryExtension> Geometry::getExtension(Base::Type type) const
{
    for( auto ext : extensions) {
        if(ext->getTypeId() == type)
            return ext;
    }

    throw Base::ValueError("No geometry extension of the requested type.");
}

const std::weak_ptr<GeometryExtension> Geometry::getExtension(std::string name) const
{
    for( auto ext : extensions) {
        if(ext->getName() == name)
            return ext;
    }

    throw Base::ValueError("No geometry extension with the requested name.");
}

void Geometry::setExtension(std::unique_ptr<GeometryExtension> && geo)
{
    bool hasext=false;

    for( auto & ext : extensions) {
        // if same type and name, this modifies the existing extension.
        if( ext->getTypeId() == geo->getTypeId() &&
            ext->getName() == geo->getName()){
            ext = std::move(geo);
            hasext = true;
        }
    }

    if(!hasext) // new type-name unique id, so add.
        extensions.push_back(std::move(geo));
}

void Geometry::deleteExtension(Base::Type type)
{
    extensions.erase(
        std::remove_if( extensions.begin(),
                        extensions.end(),
                        [&type](const std::shared_ptr<GeometryExtension>& ext){
                            return ext->getTypeId() == type;
                        }),
        extensions.end());
}

void Geometry::deleteExtension(std::string name)
{
    extensions.erase(
        std::remove_if( extensions.begin(),
                        extensions.end(),
                        [&name](const std::shared_ptr<GeometryExtension>& ext){
                            return ext->getName() == name;
                        }),
        extensions.end());
}


void Geometry::createNewTag()
{
    // Initialize a random number generator, to avoid Valgrind false positives.
    static boost::mt19937 ran;
    static bool seeded = false;

    if (!seeded) {
        ran.seed(static_cast<unsigned int>(std::time(0)));
        seeded = true;
    }
    static boost::uuids::basic_random_generator<boost::mt19937> gen(&ran);

    tag = gen();
}

void Geometry::assignTag(const Part::Geometry * geo)
{
    if(geo->getTypeId() == this->getTypeId())
        this->tag = geo->tag;
    else
        throw Base::TypeError("Geometry tag can not be assigned as geometry types do not match.");
}

Geometry *Geometry::clone(void) const
{
    Geometry* cpy = this->copy();
    cpy->tag = this->tag;

    for(auto & ext: extensions)
        cpy->extensions.push_back(std::move(ext->copy()));

    return cpy;
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomPoint,Part::Geometry)

GeomPoint::GeomPoint()
{
    this->myPoint = new Geom_CartesianPoint(0,0,0);
}

GeomPoint::GeomPoint(const Handle(Geom_CartesianPoint)& p)
{
    setHandle(p);
}

GeomPoint::GeomPoint(const Base::Vector3d& p)
{
    this->myPoint = new Geom_CartesianPoint(p.x,p.y,p.z);
}

GeomPoint::~GeomPoint()
{
}

const Handle(Geom_Geometry)& GeomPoint::handle() const
{
    return myPoint;
}


void GeomPoint::setHandle(const Handle(Geom_CartesianPoint)& p)
{
    myPoint = Handle(Geom_CartesianPoint)::DownCast(p->Copy());
}

Geometry *GeomPoint::copy(void) const
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
    return new PointPy(new GeomPoint(getPoint()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Part::GeomCurve,Part::Geometry)

GeomCurve::GeomCurve()
{
}

GeomCurve::~GeomCurve()
{
}

TopoDS_Shape GeomCurve::toShape() const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());
    BRepBuilderAPI_MakeEdge mkBuilder(c, c->FirstParameter(), c->LastParameter());
    return mkBuilder.Shape();
}

GeomBSplineCurve* GeomCurve::toBSpline(double first, double last) const
{
    ShapeConstruct_Curve scc;
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());
    Handle(Geom_BSplineCurve) spline = scc.ConvertToBSpline(c, first, last, Precision::Confusion());
    if (spline.IsNull())
        THROWM(Base::CADKernelError,"Conversion to B-spline failed")
    return new GeomBSplineCurve(spline);
}

GeomBSplineCurve* GeomCurve::toNurbs(double first, double last) const
{
    return toBSpline(first, last);
}

bool GeomCurve::tangent(double u, gp_Dir& dir) const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());
    GeomLProp_CLProps prop(c,u,1,Precision::Confusion());
    if (prop.IsTangentDefined()) {
        prop.Tangent(dir);
        return true;
    }

    return false;
}

bool GeomCurve::tangent(double u, Base::Vector3d& dir) const
{
    gp_Dir gdir;

    if (tangent(u, gdir)) {
        dir = Base::Vector3d(gdir.X(),gdir.Y(),gdir.Z());

        return true;
    }

    return false;
}

Base::Vector3d GeomCurve::pointAtParameter(double u) const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());
    GeomLProp_CLProps prop(c,u,0,Precision::Confusion());

    const gp_Pnt &point=prop.Value();

    return Base::Vector3d(point.X(),point.Y(),point.Z());
}

Base::Vector3d GeomCurve::firstDerivativeAtParameter(double u) const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());
    GeomLProp_CLProps prop(c,u,1,Precision::Confusion());

    const gp_Vec &vec=prop.D1();

    return Base::Vector3d(vec.X(),vec.Y(),vec.Z());
}

Base::Vector3d GeomCurve::secondDerivativeAtParameter(double u) const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());
    GeomLProp_CLProps prop(c,u,2,Precision::Confusion());

    const gp_Vec &vec=prop.D2();

    return Base::Vector3d(vec.X(),vec.Y(),vec.Z());
}

bool GeomCurve::normalAt(double u, Base::Vector3d& dir) const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());

    try {
        if (!c.IsNull()) {
            GeomLProp_CLProps prop(c,u,2,Precision::Confusion());
            gp_Dir gdir;
            prop.Normal(gdir);
            dir = Base::Vector3d(gdir.X(), gdir.Y(), gdir.Z());

            return true;
        }
    }
    catch (const LProp_NotDefined&) {
        dir.Set(0,0,0);
        return false;
    }
    catch (Standard_Failure& e) {
        THROWM(Base::CADKernelError,e.GetMessageString())
    }

    return false;
}

bool GeomCurve::intersect(  GeomCurve * c,
                            std::vector<std::pair<Base::Vector3d, Base::Vector3d>>& points,
                            double tol) const
{
    Handle(Geom_Curve) curve1 = Handle(Geom_Curve)::DownCast(handle());
    Handle(Geom_Curve) curve2 = Handle(Geom_Curve)::DownCast(c->handle());

    if(!curve1.IsNull() && !curve2.IsNull()) {
        return intersect(curve1,curve2,points, tol);
    }
    else
        return false;

}

bool GeomCurve::intersect(const Handle(Geom_Curve) curve1, const Handle(Geom_Curve) curve2,
                std::vector<std::pair<Base::Vector3d, Base::Vector3d>>& points,
                double tol)
{
    // https://forum.freecadweb.org/viewtopic.php?f=10&t=31700
    if (curve1->IsKind(STANDARD_TYPE(Geom_BoundedCurve)) &&
        curve2->IsKind(STANDARD_TYPE(Geom_BoundedCurve))){

        Handle(Geom_BoundedCurve) bcurve1 = Handle(Geom_BoundedCurve)::DownCast(curve1);
        Handle(Geom_BoundedCurve) bcurve2 = Handle(Geom_BoundedCurve)::DownCast(curve2);

        gp_Pnt c1s = bcurve1->StartPoint();
        gp_Pnt c2s = bcurve2->StartPoint();
        gp_Pnt c1e = bcurve1->EndPoint();
        gp_Pnt c2e = bcurve2->EndPoint();

        auto checkendpoints = [&points,tol]( gp_Pnt p1, gp_Pnt p2) {
            if(p1.Distance(p2) < tol)
                points.emplace_back(Base::Vector3d(p1.X(),p1.Y(),p1.Z()),Base::Vector3d(p2.X(),p2.Y(),p2.Z()));
        };

        checkendpoints(c1s,c2s);
        checkendpoints(c1s,c2e);
        checkendpoints(c1e,c2s);
        checkendpoints(c1e,c2e);

    }

    try {

        GeomAPI_ExtremaCurveCurve intersector(curve1, curve2);

        if (intersector.NbExtrema() == 0 || intersector.LowerDistance() > tol) {
            // No intersection
            return false;
        }

        for (int i = 1; i <= intersector.NbExtrema(); i++) {
            if (intersector.Distance(i) > tol)
                continue;

            gp_Pnt p1, p2;
            intersector.Points(i, p1, p2);
            points.emplace_back(Base::Vector3d(p1.X(),p1.Y(),p1.Z()),Base::Vector3d(p2.X(),p2.Y(),p2.Z()));
        }
    }
    catch (Standard_Failure& e) {
        // Yes Extrema finding failed, but if we got an intersection then go on with it
        if(points.size()>0)
            return points.size()>0?true:false;
        else
            THROWM(Base::CADKernelError,e.GetMessageString())
    }


    return points.size()>0?true:false;
}

bool GeomCurve::closestParameter(const Base::Vector3d& point, double &u) const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());
    try {
        if (!c.IsNull()) {
            gp_Pnt pnt(point.x,point.y,point.z);
            GeomAPI_ProjectPointOnCurve ppc(pnt, c);
            u = ppc.LowerDistanceParameter();
            return true;
        }
    }
    catch (StdFail_NotDone& e) {
        if (c->IsKind(STANDARD_TYPE(Geom_BoundedCurve))){
            Base::Vector3d firstpoint = this->pointAtParameter(c->FirstParameter());
            Base::Vector3d lastpoint = this->pointAtParameter(c->LastParameter());

            if((firstpoint-point).Length() < (lastpoint-point).Length())
                u = c->FirstParameter();
            else
                u = c->LastParameter();
        }
        else
            THROWM(Base::CADKernelError,e.GetMessageString())

        return true;
    }
    catch (Standard_Failure& e) {
        THROWM(Base::CADKernelError,e.GetMessageString())
    }

    return false;
}

bool GeomCurve::closestParameterToBasisCurve(const Base::Vector3d& point, double &u) const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());

    if (c->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))){
        Handle(Geom_TrimmedCurve) tc = Handle(Geom_TrimmedCurve)::DownCast(handle());
        Handle(Geom_Curve) bc = tc->BasisCurve();
        try {
            if (!bc.IsNull()) {
                gp_Pnt pnt(point.x,point.y,point.z);
                GeomAPI_ProjectPointOnCurve ppc(pnt, bc);
                u = ppc.LowerDistanceParameter();
                return true;
            }
        }
        catch (Standard_Failure& e) {
            THROWM(Base::CADKernelError,e.GetMessageString())
        }

        return false;
    }
    else {
        return this->closestParameter(point, u);
    }
}

double GeomCurve::getFirstParameter() const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());

    try {
        // pending check for RealFirst RealLast in case of infinite curve
        return c->FirstParameter();
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

double GeomCurve::getLastParameter() const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());

    try {
        // pending check for RealFirst RealLast in case of infinite curve
        return c->LastParameter();
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

double GeomCurve::curvatureAt(double u) const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());

    try {
        GeomLProp_CLProps prop(c,u,2,Precision::Confusion());
        return prop.Curvature();
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

double GeomCurve::length(double u, double v) const
{

    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());

    try {
        GeomAdaptor_Curve adaptor(c);
        return GCPnts_AbscissaPoint::Length(adaptor,u,v,Precision::Confusion());
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

void GeomCurve::reverse(void)
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());

    try {
        c->Reverse();
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}


// -------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Part::GeomBoundedCurve, Part::GeomCurve)

GeomBoundedCurve::GeomBoundedCurve()
{
}

GeomBoundedCurve::~GeomBoundedCurve()
{
}

Base::Vector3d GeomBoundedCurve::getStartPoint() const
{
    Handle(Geom_BoundedCurve) curve =  Handle(Geom_BoundedCurve)::DownCast(handle());
    gp_Pnt pnt = curve->StartPoint();

    return Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
}

Base::Vector3d GeomBoundedCurve::getEndPoint() const
{
    Handle(Geom_BoundedCurve) curve =  Handle(Geom_BoundedCurve)::DownCast(handle());
    gp_Pnt pnt = curve->EndPoint();

    return Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
}


// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomBezierCurve,Part::GeomBoundedCurve)

GeomBezierCurve::GeomBezierCurve()
{
    TColgp_Array1OfPnt poles(1,2);
    poles(1) = gp_Pnt(0.0,0.0,0.0);
    poles(2) = gp_Pnt(0.0,0.0,1.0);
    Handle(Geom_BezierCurve) b = new Geom_BezierCurve(poles);
    this->myCurve = b;
}

GeomBezierCurve::GeomBezierCurve(const Handle(Geom_BezierCurve)& b)
{
    setHandle(b);
}

GeomBezierCurve::GeomBezierCurve( const std::vector<Base::Vector3d>& poles, const std::vector<double>& weights)
{
    if (poles.size() != weights.size())
        throw Base::ValueError("poles and weights mismatch");

    TColgp_Array1OfPnt p(1,poles.size());
    TColStd_Array1OfReal w(1,poles.size());
    for (std::size_t i = 1; i <= poles.size(); i++) {
        p.SetValue(i, gp_Pnt(poles[i-1].x,poles[i-1].y,poles[i-1].z));
        w.SetValue(i, weights[i-1]);
    }
    this->myCurve = new Geom_BezierCurve (p, w);
}

GeomBezierCurve::~GeomBezierCurve()
{
}

void GeomBezierCurve::setHandle(const Handle(Geom_BezierCurve)& c)
{
    myCurve = Handle(Geom_BezierCurve)::DownCast(c->Copy());
}

const Handle(Geom_Geometry)& GeomBezierCurve::handle() const
{
    return myCurve;
}

Geometry *GeomBezierCurve::copy(void) const
{
    GeomBezierCurve *newCurve = new GeomBezierCurve(myCurve);
    newCurve->Construction = this->Construction;
    return newCurve;
}

std::vector<Base::Vector3d> GeomBezierCurve::getPoles() const
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

std::vector<double> GeomBezierCurve::getWeights() const
{
    std::vector<double> weights;
    weights.reserve(myCurve->NbPoles());
    TColStd_Array1OfReal w(1,myCurve->NbPoles());
    myCurve->Weights(w);

    for (Standard_Integer i=w.Lower(); i<=w.Upper(); i++) {
        const Standard_Real& real = w(i);
        weights.push_back(real);
    }
    return weights;
}

// Persistence implementer
unsigned int GeomBezierCurve::getMemSize (void) const
{
    return sizeof(Geom_BezierCurve);
}

void GeomBezierCurve::Save(Base::Writer& writer) const
{
    // save the attributes of the father class
    GeomCurve::Save(writer);

    std::vector<Base::Vector3d> poles   = this->getPoles();
    std::vector<double> weights         = this->getWeights();

    writer.Stream()
         << writer.ind()
             << "<BezierCurve "
                << "PolesCount=\"" <<  poles.size() <<
             "\">" << endl;

    writer.incInd();

    std::vector<Base::Vector3d>::const_iterator itp;
    std::vector<double>::const_iterator itw;

    for (itp = poles.begin(), itw = weights.begin(); itp != poles.end() && itw != weights.end(); ++itp, ++itw) {
        writer.Stream()
            << writer.ind()
            << "<Pole "
            << "X=\"" << (*itp).x <<
            "\" Y=\"" << (*itp).y <<
            "\" Z=\"" << (*itp).z <<
            "\" Weight=\"" << (*itw) <<
        "\"/>" << endl;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</BezierCurve>" << endl ;
}

void GeomBezierCurve::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    GeomCurve::Restore(reader);

    reader.readElement("BezierCurve");
    // get the value of my attribute
    int polescount = reader.getAttributeAsInteger("PolesCount");

    TColgp_Array1OfPnt p(1,polescount);
    TColStd_Array1OfReal w(1,polescount);

    for (int i = 1; i <= polescount; i++) {
        reader.readElement("Pole");
        double X = reader.getAttributeAsFloat("X");
        double Y = reader.getAttributeAsFloat("Y");
        double Z = reader.getAttributeAsFloat("Z");
        double W = reader.getAttributeAsFloat("Weight");
        p.SetValue(i, gp_Pnt(X,Y,Z));
        w.SetValue(i, W);
    }

    reader.readEndElement("BezierCurve");

    try {
        Handle(Geom_BezierCurve) bezier = new Geom_BezierCurve(p, w);

        if (!bezier.IsNull())
            this->myCurve = bezier;
        else
            THROWM(Base::CADKernelError,"BezierCurve restore failed")
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

PyObject *GeomBezierCurve::getPyObject(void)
{
    return new BezierCurvePy((GeomBezierCurve*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomBSplineCurve,Part::GeomBoundedCurve)

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

GeomBSplineCurve::GeomBSplineCurve(const Handle(Geom_BSplineCurve)& b)
{
    setHandle(b);
}

GeomBSplineCurve::GeomBSplineCurve( const std::vector<Base::Vector3d>& poles, const std::vector<double>& weights,
                  const std::vector<double>& knots, const std::vector<int>& multiplicities,
                  int degree, bool periodic, bool checkrational)
{
    if (poles.size() != weights.size())
        throw Base::ValueError("poles and weights mismatch");

    if (knots.size() != multiplicities.size())
        throw Base::ValueError("knots and multiplicities mismatch");

    TColgp_Array1OfPnt p(1,poles.size());
    TColStd_Array1OfReal w(1,poles.size());
    TColStd_Array1OfReal k(1,knots.size());
    TColStd_Array1OfInteger m(1,knots.size());

    for (std::size_t i = 1; i <= poles.size(); i++) {
        p.SetValue(i, gp_Pnt(poles[i-1].x,poles[i-1].y,poles[i-1].z));
        w.SetValue(i, weights[i-1]);
    }

    for (std::size_t i = 1; i <= knots.size(); i++) {
        k.SetValue(i, knots[i-1]);
        m.SetValue(i, multiplicities[i-1]);
    }

    this->myCurve = new Geom_BSplineCurve (p, w, k, m, degree, periodic?Standard_True:Standard_False, checkrational?Standard_True:Standard_False);

}


GeomBSplineCurve::~GeomBSplineCurve()
{
}

void GeomBSplineCurve::setHandle(const Handle(Geom_BSplineCurve)& c)
{
    myCurve = Handle(Geom_BSplineCurve)::DownCast(c->Copy());
}

const Handle(Geom_Geometry)& GeomBSplineCurve::handle() const
{
    return myCurve;
}

Geometry *GeomBSplineCurve::copy(void) const
{
    GeomBSplineCurve *newCurve = new GeomBSplineCurve(myCurve);
    newCurve->Construction = this->Construction;
    return newCurve;
}

int GeomBSplineCurve::countPoles() const
{
    return myCurve->NbPoles();
}

int GeomBSplineCurve::countKnots() const
{
    return myCurve->NbKnots();
}

void GeomBSplineCurve::setPole(int index, const Base::Vector3d& pole, double weight)
{
    try {
        gp_Pnt pnt(pole.x,pole.y,pole.z);
        if (weight < 0.0)
            myCurve->SetPole(index,pnt);
        else
            myCurve->SetPole(index,pnt,weight);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

void GeomBSplineCurve::setPoles(const std::vector<Base::Vector3d>& poles, const std::vector<double>& weights)
{
    if (poles.size() != weights.size())
        throw Base::ValueError("poles and weights mismatch");

    Standard_Integer index=1;

    for (std::size_t it = 0; it < poles.size(); it++, index++) {
        setPole(index, poles[it], weights[it]);
    }
}

void GeomBSplineCurve::setPoles(const std::vector<Base::Vector3d>& poles)
{
    Standard_Integer index=1;

    for (std::vector<Base::Vector3d>::const_iterator it = poles.begin(); it != poles.end(); ++it, index++){
        setPole(index, *it);
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

std::vector<double> GeomBSplineCurve::getWeights() const
{
    std::vector<double> weights;
    weights.reserve(myCurve->NbPoles());
    TColStd_Array1OfReal w(1,myCurve->NbPoles());
    myCurve->Weights(w);

    for (Standard_Integer i=w.Lower(); i<=w.Upper(); i++) {
        const Standard_Real& real = w(i);
        weights.push_back(real);
    }
    return weights;
}

void GeomBSplineCurve::setWeights(const std::vector<double>& weights)
{
    try {
        Standard_Integer index=1;

        for (std::vector<double>::const_iterator it = weights.begin(); it != weights.end(); ++it, index++){
            myCurve->SetWeight(index, *it);
        }
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

void GeomBSplineCurve::setKnot(int index, const double val, int mult)
{
    try {
        if (mult < 0)
            myCurve->SetKnot(index, val);
        else
            myCurve->SetKnot(index, val, mult);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

void GeomBSplineCurve::setKnots(const std::vector<double>& knots)
{
    Standard_Integer index=1;

    for (std::vector<double>::const_iterator it = knots.begin(); it != knots.end(); ++it, index++) {
        setKnot(index, *it);
    }
}

void GeomBSplineCurve::setKnots(const std::vector<double>& knots, const std::vector<int>& multiplicities)
{
    if (knots.size() != multiplicities.size())
        throw Base::ValueError("knots and multiplicities mismatch");

    Standard_Integer index=1;

    for (std::size_t it = 0; it < knots.size(); it++, index++) {
        setKnot(index, knots[it], multiplicities[it]);
    }
}

std::vector<double> GeomBSplineCurve::getKnots() const
{
    std::vector<double> knots;
    knots.reserve(myCurve->NbKnots());
    TColStd_Array1OfReal k(1,myCurve->NbKnots());
    myCurve->Knots(k);

    for (Standard_Integer i=k.Lower(); i<=k.Upper(); i++) {
        const Standard_Real& real = k(i);
        knots.push_back(real);
    }
    return knots;
}

std::vector<int> GeomBSplineCurve::getMultiplicities() const
{
    std::vector<int> mults;
    mults.reserve(myCurve->NbKnots());
    TColStd_Array1OfInteger m(1,myCurve->NbKnots());
    myCurve->Multiplicities(m);

    for (Standard_Integer i=m.Lower(); i<=m.Upper(); i++) {
        const Standard_Integer& nm = m(i);
        mults.push_back(nm);
    }
    return mults;
}

int GeomBSplineCurve::getMultiplicity(int index) const
{
    try {
        return myCurve->Multiplicity(index);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

int GeomBSplineCurve::getDegree() const
{
    return myCurve->Degree();
}

bool GeomBSplineCurve::isPeriodic() const
{
    return myCurve->IsPeriodic()==Standard_True;
}

bool GeomBSplineCurve::join(const Handle(Geom_BSplineCurve)& spline)
{
    GeomConvert_CompCurveToBSplineCurve ccbc(this->myCurve);
    if (!ccbc.Add(spline, Precision::Approximation()))
        return false;
    this->myCurve = ccbc.BSplineCurve();
    return true;
}

void GeomBSplineCurve::interpolate(const std::vector<gp_Pnt>& p,
                                   const std::vector<gp_Vec>& t)
{
    if (p.size() < 2)
        Standard_ConstructionError::Raise();
    if (p.size() != t.size())
        Standard_ConstructionError::Raise();

    double tol3d = Precision::Approximation();
    Handle(TColgp_HArray1OfPnt) pts = new TColgp_HArray1OfPnt(1, p.size());
    for (std::size_t i=0; i<p.size(); i++) {
        pts->SetValue(i+1, p[i]);
    }

    TColgp_Array1OfVec tgs(1, t.size());
    Handle(TColStd_HArray1OfBoolean) fgs = new TColStd_HArray1OfBoolean(1, t.size());
    for (std::size_t i=0; i<p.size(); i++) {
        tgs.SetValue(i+1, t[i]);
        fgs->SetValue(i+1, Standard_True);
    }

    GeomAPI_Interpolate interpolate(pts, Standard_False, tol3d);
    interpolate.Load(tgs, fgs);
    interpolate.Perform();
    this->myCurve = interpolate.Curve();
}

void GeomBSplineCurve::getCardinalSplineTangents(const std::vector<gp_Pnt>& p,
                                                 const std::vector<double>& c,
                                                 std::vector<gp_Vec>& t) const
{
    // https://de.wikipedia.org/wiki/Kubisch_Hermitescher_Spline#Cardinal_Spline
    if (p.size() < 2)
        Standard_ConstructionError::Raise();
    if (p.size() != c.size())
        Standard_ConstructionError::Raise();

    t.resize(p.size());
    if (p.size() == 2) {
        t[0] = gp_Vec(p[0], p[1]);
        t[1] = gp_Vec(p[0], p[1]);
    }
    else {
        std::size_t e = p.size() - 1;

        for (std::size_t i = 1; i < e; i++) {
            gp_Vec v = gp_Vec(p[i-1], p[i+1]);
            double f = 0.5 * (1-c[i]);
            v.Scale(f);
            t[i] = v;
        }

        t[0] = t[1];
        t[t.size()-1] = t[t.size()-2];
    }
}

void GeomBSplineCurve::getCardinalSplineTangents(const std::vector<gp_Pnt>& p, double c,
                                                 std::vector<gp_Vec>& t) const
{
    // https://de.wikipedia.org/wiki/Kubisch_Hermitescher_Spline#Cardinal_Spline
    if (p.size() < 2)
        Standard_ConstructionError::Raise();

    t.resize(p.size());
    if (p.size() == 2) {
        t[0] = gp_Vec(p[0], p[1]);
        t[1] = gp_Vec(p[0], p[1]);
    }
    else {
        std::size_t e = p.size() - 1;
        double f = 0.5 * (1-c);

        for (std::size_t i = 1; i < e; i++) {
            gp_Vec v = gp_Vec(p[i-1], p[i+1]);
            v.Scale(f);
            t[i] = v;
        }

        t[0] = t[1];
        t[t.size()-1] = t[t.size()-2];
    }
}

void GeomBSplineCurve::makeC1Continuous(double tol, double ang_tol)
{
    GeomConvert::C0BSplineToC1BSplineCurve(this->myCurve, tol, ang_tol);
}

void GeomBSplineCurve::increaseDegree(double degree)
{
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(this->handle());
        curve->IncreaseDegree(degree);
        return;
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

void GeomBSplineCurve::increaseMultiplicity(int index, int multiplicity)
{
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(this->handle());
        curve->IncreaseMultiplicity(index, multiplicity);
        return;
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

bool GeomBSplineCurve::removeKnot(int index, int multiplicity, double tolerance)
{
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(this->handle());
        return curve->RemoveKnot(index, multiplicity, tolerance) == Standard_True;
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}


// Persistence implementer
unsigned int GeomBSplineCurve::getMemSize (void) const
{
    return sizeof(Geom_BSplineCurve);
}

void GeomBSplineCurve::Save(Base::Writer& writer) const
{
    // save the attributes of the father class
    GeomCurve::Save(writer);

    std::vector<Base::Vector3d> poles   = this->getPoles();
    std::vector<double> weights         = this->getWeights();
    std::vector<double> knots           = this->getKnots();
    std::vector<int> mults              = this->getMultiplicities();
    int degree                          = this->getDegree();
    bool isperiodic                     = this->isPeriodic();

    writer.Stream()
         << writer.ind()
             << "<BSplineCurve "
                << "PolesCount=\"" <<  poles.size() <<
                 "\" KnotsCount=\"" <<  knots.size() <<
                 "\" Degree=\"" <<  degree <<
                 "\" IsPeriodic=\"" <<  (int) isperiodic <<
             "\">" << endl;

    writer.incInd();

    std::vector<Base::Vector3d>::const_iterator itp;
    std::vector<double>::const_iterator itw;

    for (itp = poles.begin(), itw = weights.begin(); itp != poles.end() && itw != weights.end(); ++itp, ++itw) {
        writer.Stream()
            << writer.ind()
            << "<Pole "
            << "X=\"" << (*itp).x <<
            "\" Y=\"" << (*itp).y <<
            "\" Z=\"" << (*itp).z <<
            "\" Weight=\"" << (*itw) <<
        "\"/>" << endl;
    }

    std::vector<double>::const_iterator itk;
    std::vector<int>::const_iterator itm;

    for (itk = knots.begin(), itm = mults.begin(); itk != knots.end() && itm != mults.end(); ++itk, ++itm) {
        writer.Stream()
            << writer.ind()
            << "<Knot "
            << "Value=\"" << (*itk)
            << "\" Mult=\"" << (*itm) <<
        "\"/>" << endl;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</BSplineCurve>" << endl ;
}

void GeomBSplineCurve::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    GeomCurve::Restore(reader);

    reader.readElement("BSplineCurve");
    // get the value of my attribute
    int polescount = reader.getAttributeAsInteger("PolesCount");
    int knotscount = reader.getAttributeAsInteger("KnotsCount");
    int degree = reader.getAttributeAsInteger("Degree");
    bool isperiodic = (bool) reader.getAttributeAsInteger("IsPeriodic");

    // Handle(Geom_BSplineCurve) spline = new
    // Geom_BSplineCurve(occpoles,occweights,occknots,occmults,degree,
    // PyObject_IsTrue(periodic) ? Standard_True : Standard_False,
    // PyObject_IsTrue(CheckRational) ? Standard_True : Standard_False);

    TColgp_Array1OfPnt p(1,polescount);
    TColStd_Array1OfReal w(1,polescount);
    TColStd_Array1OfReal k(1,knotscount);
    TColStd_Array1OfInteger m(1,knotscount);

    for (int i = 1; i <= polescount; i++) {
        reader.readElement("Pole");
        double X = reader.getAttributeAsFloat("X");
        double Y = reader.getAttributeAsFloat("Y");
        double Z = reader.getAttributeAsFloat("Z");
        double W = reader.getAttributeAsFloat("Weight");
        p.SetValue(i, gp_Pnt(X,Y,Z));
        w.SetValue(i, W);
    }

    for (int i = 1; i <= knotscount; i++) {
        reader.readElement("Knot");
        double val = reader.getAttributeAsFloat("Value");
        Standard_Integer mult = reader.getAttributeAsInteger("Mult");
        k.SetValue(i, val);
        m.SetValue(i, mult);
    }

    reader.readEndElement("BSplineCurve");
    // Geom_BSplineCurve(occpoles,occweights,occknots,occmults,degree,periodic,CheckRational

    try {
        Handle(Geom_BSplineCurve) spline = new Geom_BSplineCurve(p, w, k, m, degree, isperiodic ? Standard_True : Standard_False, Standard_False);

        if (!spline.IsNull())
            this->myCurve = spline;
        else
            THROWM(Base::CADKernelError,"BSpline restore failed")
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}


PyObject *GeomBSplineCurve::getPyObject(void)
{
    return new BSplineCurvePy((GeomBSplineCurve*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Part::GeomConic, Part::GeomCurve)

GeomConic::GeomConic()
{
}

GeomConic::~GeomConic()
{
}

Base::Vector3d GeomConic::getLocation(void) const
{
    Handle(Geom_Conic) conic =  Handle(Geom_Conic)::DownCast(handle());
    gp_Ax1 axis = conic->Axis();
    const gp_Pnt& loc = axis.Location();
    return Base::Vector3d(loc.X(),loc.Y(),loc.Z());
}

void GeomConic::setLocation(const Base::Vector3d& Center)
{
    gp_Pnt p1(Center.x,Center.y,Center.z);
    Handle(Geom_Conic) conic =  Handle(Geom_Conic)::DownCast(handle());

    try {
        conic->SetLocation(p1);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

Base::Vector3d GeomConic::getCenter(void) const
{
    Handle(Geom_Conic) conic =  Handle(Geom_Conic)::DownCast(handle());
    gp_Ax1 axis = conic->Axis();
    const gp_Pnt& loc = axis.Location();
    return Base::Vector3d(loc.X(),loc.Y(),loc.Z());
}

void GeomConic::setCenter(const Base::Vector3d& Center)
{
    gp_Pnt p1(Center.x,Center.y,Center.z);
    Handle(Geom_Conic) conic =  Handle(Geom_Conic)::DownCast(handle());

    try {
        conic->SetLocation(p1);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

/*!
 * \brief GeomConic::getAngleXU
 * \return The angle between ellipse's major axis (in direction to focus1) and
 * X axis of a default axis system in the plane of ellipse. The angle is
 * counted CCW as seen when looking at the ellipse so that ellipse's axis is
 * pointing at you. Note that this function may give unexpected results when
 * the ellipse is in XY, but reversed, because the X axis of the default axis
 * system is reversed compared to the global X axis. This angle, in conjunction
 * with ellipse's axis, fully defines the orientation of the ellipse.
 */
double GeomConic::getAngleXU(void) const
{
    Handle(Geom_Conic) conic =  Handle(Geom_Conic)::DownCast(handle());

    gp_Pnt center = conic->Axis().Location();
    gp_Dir normal = conic->Axis().Direction();
    gp_Dir xdir = conic->XAxis().Direction();


    gp_Ax2 xdirref(center, normal); // this is a reference system, might be CCW or CW depending on the creation method

    return -xdir.AngleWithRef(xdirref.XDirection(),normal);

}

/*!
 * \brief GeomConic::setAngleXU complements getAngleXU.
 * \param angle
 */
void GeomConic::setAngleXU(double angle)
{
    Handle(Geom_Conic) conic =  Handle(Geom_Conic)::DownCast(handle());;

    try {
        gp_Pnt center = conic->Axis().Location();
        gp_Dir normal = conic->Axis().Direction();

        gp_Ax1 normaxis(center, normal);
        gp_Ax2 xdirref(center, normal);

        xdirref.Rotate(normaxis,angle);
        conic->SetPosition(xdirref);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

/*!
 * \brief GeomConic::isReversed tests if an ellipse that lies in XY plane
 * is reversed (i.e. drawn from startpoint to endpoint in CW direction instead
 * of CCW.)
 * \return Returns True if the arc is CW and false if CCW.
 */
bool GeomConic::isReversed() const
{
    Handle(Geom_Conic) conic =  Handle(Geom_Conic)::DownCast(handle());
    assert(!conic.IsNull());
    return conic->Axis().Direction().Z() < 0;
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomTrimmedCurve,Part::GeomBoundedCurve)

GeomTrimmedCurve::GeomTrimmedCurve()
{
}

GeomTrimmedCurve::GeomTrimmedCurve(const Handle(Geom_TrimmedCurve)& c)
{
    setHandle(c);
}

GeomTrimmedCurve::~GeomTrimmedCurve()
{
}

void GeomTrimmedCurve::setHandle(const Handle(Geom_TrimmedCurve)& c)
{
    this->myCurve = Handle(Geom_TrimmedCurve)::DownCast(c->Copy());
}

const Handle(Geom_Geometry)& GeomTrimmedCurve::handle() const
{
    return myCurve;
}

Geometry *GeomTrimmedCurve::copy(void) const
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

bool GeomTrimmedCurve::intersectBasisCurves(  const GeomTrimmedCurve * c,
                                std::vector<std::pair<Base::Vector3d, Base::Vector3d>>& points,
                                double tol) const
{
    Handle(Geom_TrimmedCurve) curve1 =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_TrimmedCurve) curve2 =  Handle(Geom_TrimmedCurve)::DownCast(c->handle());

    Handle(Geom_Curve) bcurve1 = curve1->BasisCurve();
    Handle(Geom_Curve) bcurve2 = curve2->BasisCurve();


    if(!bcurve1.IsNull() && !bcurve2.IsNull()) {

        return intersect(bcurve1, bcurve2, points, tol);
    }
    else
        return false;

}

void GeomTrimmedCurve::getRange(double& u, double& v) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    u = curve->FirstParameter();
    v = curve->LastParameter();
}

void GeomTrimmedCurve::setRange(double u, double v)
{
    try {
        Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());

        curve->SetTrim(u, v);
    }
    catch (Standard_Failure& e) {
        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

// -------------------------------------------------
TYPESYSTEM_SOURCE_ABSTRACT(Part::GeomArcOfConic,Part::GeomTrimmedCurve)

GeomArcOfConic::GeomArcOfConic()
{
}

GeomArcOfConic::~GeomArcOfConic()
{
}

/*!
 * \brief GeomArcOfConic::getStartPoint
 * \param emulateCCWXY: if true, the arc will pretent to be a CCW arc in XY plane.
 * For this to work, the arc must lie in XY plane (i.e. Axis is either +Z or -Z).
 * \return XYZ of the arc's starting point.
 */
Base::Vector3d GeomArcOfConic::getStartPoint(bool emulateCCWXY) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    gp_Pnt pnt = curve->StartPoint();
    if (emulateCCWXY) {
        if (isReversed())
            pnt = curve->EndPoint();
    }
    return Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
}

/*!
 * \brief GeomArcOfConic::getEndPoint
 * \param emulateCCWXY: if true, the arc will pretent to be a CCW arc in XY plane.
 * For this to work, the arc must lie in XY plane (i.e. Axis is either +Z or -Z).
 * \return
 */
Base::Vector3d GeomArcOfConic::getEndPoint(bool emulateCCWXY) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    gp_Pnt pnt = curve->EndPoint();
    if (emulateCCWXY) {
        if (isReversed())
            pnt = curve->StartPoint();
    }
    return Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
}

Base::Vector3d GeomArcOfConic::getCenter(void) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve->BasisCurve());
    gp_Ax1 axis = conic->Axis();
    const gp_Pnt& loc = axis.Location();
    return Base::Vector3d(loc.X(),loc.Y(),loc.Z());
}

Base::Vector3d GeomArcOfConic::getLocation(void) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve->BasisCurve());
    gp_Ax1 axis = conic->Axis();
    const gp_Pnt& loc = axis.Location();
    return Base::Vector3d(loc.X(),loc.Y(),loc.Z());
}

void GeomArcOfConic::setCenter(const Base::Vector3d& Center)
{
    gp_Pnt p1(Center.x,Center.y,Center.z);
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve->BasisCurve());

    try {
        conic->SetLocation(p1);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

void GeomArcOfConic::setLocation(const Base::Vector3d& Center)
{
    gp_Pnt p1(Center.x,Center.y,Center.z);
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve->BasisCurve());

    try {
        conic->SetLocation(p1);
    }
    catch (Standard_Failure& e) {

       THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

/*!
 * \brief GeomArcOfConic::isReversed
 * \return tests if an arc that lies in XY plane is reversed (i.e. drawn from
 * startpoint to endpoint in CW direction instead of CCW.). Returns True if the
 * arc is CW and false if CCW.
 */
bool GeomArcOfConic::isReversed() const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve->BasisCurve());
    assert(!conic.IsNull());
    return conic->Axis().Direction().Z() < 0;
}

/*!
 * \brief GeomArcOfConic::getAngleXU
 * \return The angle between ellipse's major axis (in direction to focus1) and
 * X axis of a default axis system in the plane of ellipse. The angle is
 * counted CCW as seen when looking at the ellipse so that ellipse's axis is
 * pointing at you. Note that this function may give unexpected results when
 * the ellipse is in XY, but reversed, because the X axis of the default axis
 * system is reversed compared to the global X axis. This angle, in conjunction
 * with ellipse's axis, fully defines the orientation of the ellipse.
 */
double GeomArcOfConic::getAngleXU(void) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve->BasisCurve());

    gp_Pnt center = conic->Axis().Location();
    gp_Dir normal = conic->Axis().Direction();
    gp_Dir xdir = conic->XAxis().Direction();

    gp_Ax2 xdirref(center, normal); // this is a reference system, might be CCW or CW depending on the creation method

    return -xdir.AngleWithRef(xdirref.XDirection(),normal);
}

/*!
 * \brief GeomArcOfConic::setAngleXU complements getAngleXU.
 */
void GeomArcOfConic::setAngleXU(double angle)
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve->BasisCurve());

    try {
        gp_Pnt center = conic->Axis().Location();
        gp_Dir normal = conic->Axis().Direction();

        gp_Ax1 normaxis(center, normal);
        gp_Ax2 xdirref(center, normal);

        xdirref.Rotate(normaxis,angle);
        conic->SetPosition(xdirref);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

/*!
 * \brief GeomArcOfConic::getXAxisDir
 * \return the direction vector (unit-length) of symmetry axis of the conic. The
 * direction also points to the focus of a parabola.
 */
Base::Vector3d GeomArcOfConic::getXAxisDir() const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Conic) c = Handle(Geom_Conic)::DownCast( curve->BasisCurve() );
    assert(!c.IsNull());
    gp_Dir xdir = c->XAxis().Direction();
    return Base::Vector3d(xdir.X(), xdir.Y(), xdir.Z());
}

/*!
 * \brief GeomArcOfConic::setXAxisDir Rotates the conic in its plane, so
 * that its symmetry axis is as close as possible to the provided direction.
 * \param newdir [in] is the new direction. If the vector is small, the
 * orientation of the conic will be preserved. If the vector is not small,
 * but its projection onto plane of the conic is small, an exception will be
 * thrown.
 */
void GeomArcOfConic::setXAxisDir(const Base::Vector3d& newdir)
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Conic) c = Handle(Geom_Conic)::DownCast( curve->BasisCurve() );
    assert(!c.IsNull());
#if OCC_VERSION_HEX >= 0x060504
    if (newdir.Sqr() < Precision::SquareConfusion())
#else
    if (newdir.Length() < Precision::Confusion())
#endif
        return;//zero vector was passed. Keep the old orientation.

    try {
        gp_Ax2 pos = c->Position();
        //OCC should keep the old main Direction (Z), and change YDirection to accommodate the new XDirection.
        pos.SetXDirection(gp_Dir(newdir.x, newdir.y, newdir.z));
        c->SetPosition(pos);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomCircle,Part::GeomConic)

GeomCircle::GeomCircle()
{
    Handle(Geom_Circle) c = new Geom_Circle(gp_Circ());
    this->myCurve = c;
}

GeomCircle::GeomCircle(const Handle(Geom_Circle)& c)
{
    setHandle(c);
}

GeomCircle::~GeomCircle()
{
}

const Handle(Geom_Geometry)& GeomCircle::handle() const
{
    return myCurve;
}


void GeomCircle::setHandle(const Handle(Geom_Circle)& c)
{
    myCurve = Handle(Geom_Circle)::DownCast(c->Copy());
}

Geometry *GeomCircle::copy(void) const
{
    GeomCircle *newCirc = new GeomCircle(myCurve);
    newCirc->Construction = this->Construction;
    return newCirc;
}

GeomBSplineCurve* GeomCircle::toNurbs(double first, double last) const
{
    double radius = getRadius();
    Handle(Geom_Conic) conic =  Handle(Geom_Conic)::DownCast(handle());
    gp_Ax1 axis = conic->Axis();
  //gp_Dir xdir = conic->XAxis().Direction();
  //Standard_Real angle = gp_Dir(1,0,0).Angle(xdir) + first;
    Standard_Real angle = first;
    const gp_Pnt& loc = axis.Location();
    //Note: If the matching this way doesn't work reliably then we must compute the
    //angle so that the point of the curve for 'first' matches the first pole
    //gp_Pnt pnt = conic->Value(first);

    TColgp_Array1OfPnt poles(1, 7);
    poles(1) = loc.Translated(gp_Vec(radius, 0, 0));
    poles(2) = loc.Translated(gp_Vec(radius, 2*radius, 0));
    poles(3) = loc.Translated(gp_Vec(-radius, 2*radius, 0));
    poles(4) = loc.Translated(gp_Vec(-radius, 0, 0));
    poles(5) = loc.Translated(gp_Vec(-radius, -2*radius, 0));
    poles(6) = loc.Translated(gp_Vec(radius, -2*radius, 0));
    poles(7) = loc.Translated(gp_Vec(radius, 0, 0));

    TColStd_Array1OfReal weights(1,7);
    for (int i=1; i<=7; i++) {
        poles(i).Rotate(axis, angle);
        weights(i) = 1;
    }
    weights(1) = 3;
    weights(4) = 3;
    weights(7) = 3;

    TColStd_Array1OfInteger mults(1, 3);
    mults(1) = 4;
    mults(2) = 3;
    mults(3) = 4;

    TColStd_Array1OfReal knots(1, 3);
    knots(1) = 0;
    knots(2) = M_PI;
    knots(3) = 2*M_PI;

    Handle(Geom_BSplineCurve) spline = new Geom_BSplineCurve(poles, weights,knots, mults, 3,
        Standard_False, Standard_True);
    spline->Segment(0, last-first);
    return new GeomBSplineCurve(spline);
}

double GeomCircle::getRadius(void) const
{
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(handle());
    return circle->Radius();
}

void GeomCircle::setRadius(double Radius)
{
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(handle());

    try {
        gp_Circ c = circle->Circ();
        c.SetRadius(Radius);
        circle->SetCirc(c);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
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
    gp_Dir normal = this->myCurve->Axis().Direction();
    gp_Dir xdir = this->myCurve->XAxis().Direction();

    gp_Ax2 xdirref(center, normal); // this is a reference XY for the circle
    double AngleXU = -xdir.AngleWithRef(xdirref.XDirection(),normal);

    writer.Stream()
         << writer.ind()
             << "<Circle "
                << "CenterX=\"" <<  center.X() <<
                "\" CenterY=\"" <<  center.Y() <<
                "\" CenterZ=\"" <<  center.Z() <<
                "\" NormalX=\"" <<  normal.X() <<
                "\" NormalY=\"" <<  normal.Y() <<
                "\" NormalZ=\"" <<  normal.Z() <<
                "\" AngleXU=\"" <<  AngleXU <<
                "\" Radius=\"" <<  this->myCurve->Radius() <<
             "\"/>" << endl;
}

void GeomCircle::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    GeomCurve::Restore(reader);

    double CenterX,CenterY,CenterZ,NormalX,NormalY,NormalZ,Radius;
    double AngleXU=0;
    // read my Element
    reader.readElement("Circle");
    // get the value of my Attribute
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    CenterZ = reader.getAttributeAsFloat("CenterZ");
    NormalX = reader.getAttributeAsFloat("NormalX");
    NormalY = reader.getAttributeAsFloat("NormalY");
    NormalZ = reader.getAttributeAsFloat("NormalZ");
    if (reader.hasAttribute("AngleXU"))
        AngleXU = reader.getAttributeAsFloat("AngleXU");
    Radius = reader.getAttributeAsFloat("Radius");

    // set the read geometry
    gp_Pnt p1(CenterX,CenterY,CenterZ);
    gp_Dir norm(NormalX,NormalY,NormalZ);

    gp_Ax1 normaxis(p1,norm);
    gp_Ax2 xdir(p1, norm);
    xdir.Rotate(normaxis,AngleXU);

    try {
        GC_MakeCircle mc(xdir, Radius);
        if (!mc.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(mc.Status()))

        this->myCurve = mc.Value();
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

PyObject *GeomCircle::getPyObject(void)
{
    return new CirclePy(static_cast<GeomCircle*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomArcOfCircle,Part::GeomArcOfConic)

GeomArcOfCircle::GeomArcOfCircle()
{
    Handle(Geom_Circle) c = new Geom_Circle(gp_Circ());
    this->myCurve = new Geom_TrimmedCurve(c, c->FirstParameter(),c->LastParameter());
}

GeomArcOfCircle::GeomArcOfCircle(const Handle(Geom_Circle)& c)
{
    setHandle(c);
}

GeomArcOfCircle::~GeomArcOfCircle()
{
}

void GeomArcOfCircle::setHandle(const Handle(Geom_TrimmedCurve)& c)
{
    Handle(Geom_Circle) basis = Handle(Geom_Circle)::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not a circle");
    this->myCurve = Handle(Geom_TrimmedCurve)::DownCast(c->Copy());
}

void GeomArcOfCircle::setHandle(const Handle(Geom_Circle)& c)
{
    this->myCurve = new Geom_TrimmedCurve(c, c->FirstParameter(),c->LastParameter());
}


const Handle(Geom_Geometry)& GeomArcOfCircle::handle() const
{
    return myCurve;
}

Geometry *GeomArcOfCircle::copy(void) const
{
    GeomArcOfCircle* copy = new GeomArcOfCircle();
    copy->setHandle(this->myCurve);
    copy->Construction = this->Construction;
    return copy;
}

GeomBSplineCurve* GeomArcOfCircle::toNurbs(double first, double last) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(curve->BasisCurve());
    return GeomCircle(circle).toNurbs(first, last);
}

double GeomArcOfCircle::getRadius(void) const
{
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(myCurve->BasisCurve());
    return circle->Radius();
}

void GeomArcOfCircle::setRadius(double Radius)
{
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(myCurve->BasisCurve());

    try {
        gp_Circ c = circle->Circ();
        c.SetRadius(Radius);
        circle->SetCirc(c);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

/*!
 * \brief GeomArcOfCircle::getRange
 * \param u [out] start angle of the arc, in radians.
 * \param v [out] end angle of the arc, in radians.
 * \param emulateCCWXY: if true, the arc will pretent to be a CCW arc in XY plane.
 * For this to work, the arc must lie in XY plane (i.e. Axis is either +Z or -Z).
 * Additionally, arc's rotation as a whole will be included in the returned u,v
 * (ArcOfCircle specific).
 */
void GeomArcOfCircle::getRange(double& u, double& v, bool emulateCCWXY) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    u = curve->FirstParameter();
    v = curve->LastParameter();
    if (emulateCCWXY){
        Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve->BasisCurve());
        double angleXU = -conic->Position().XDirection().AngleWithRef(gp_Dir(1.0,0.0,0.0), gp_Dir(0.0,0.0,1.0));
        double u1 = u, v1 = v;//the true arc curve parameters, cached. u,v will contain the rotation-corrected and swapped angles.
        if (conic->Axis().Direction().Z() > 0.0){
            //normal CCW arc
            u = u1 + angleXU;
            v = v1 + angleXU;
        }
        else {
            //reversed (CW) arc
            u = angleXU - v1;
            v = angleXU - u1;
        }

        if (v < u)
            v += 2*M_PI;
        if (v-u > 2*M_PI)
            v -= 2*M_PI;
    }
}

/*!
 * \brief GeomArcOfCircle::setRange
 * \param u [in] start angle of the arc, in radians.
 * \param v [in] end angle of the arc, in radians.
 * \param emulateCCWXY: if true, the arc will pretent to be a CCW arc in XY plane.
 * For this to work, the arc must lie in XY plane (i.e. Axis is either +Z or -Z).
 * Additionally, arc's rotation as a whole will be subtracted from u,v
 * (ArcOfCircle specific).
 */
void GeomArcOfCircle::setRange(double u, double v, bool emulateCCWXY)
{
    try {
        Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
        if (emulateCCWXY){
            Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve->BasisCurve());
            double angleXU = -conic->Position().XDirection().AngleWithRef(gp_Dir(1.0,0.0,0.0), gp_Dir(0.0,0.0,1.0));
            double u1 = u, v1 = v;//the values that were passed, ccw angles from X axis. u,v will contain the rotation-corrected and swapped angles.
            if (conic->Axis().Direction().Z() > 0.0){
                //normal CCW arc
                u = u1 - angleXU;
                v = v1 - angleXU;
            }
            else {
                //reversed (CW) arc
                u = angleXU - v1;
                v = angleXU - u1;
            }
        }

        curve->SetTrim(u, v);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
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

    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(this->myCurve->BasisCurve());

    gp_Pnt center = circle->Axis().Location();
    gp_Dir normal = circle->Axis().Direction();
    gp_Dir xdir = circle->XAxis().Direction();

    gp_Ax2 xdirref(center, normal); // this is a reference XY for the arc
    double AngleXU = -xdir.AngleWithRef(xdirref.XDirection(),normal);

    writer.Stream()
         << writer.ind()
             << "<ArcOfCircle "
                << "CenterX=\"" <<  center.X() <<
                "\" CenterY=\"" <<  center.Y() <<
                "\" CenterZ=\"" <<  center.Z() <<
                "\" NormalX=\"" <<  normal.X() <<
                "\" NormalY=\"" <<  normal.Y() <<
                "\" NormalZ=\"" <<  normal.Z() <<
                "\" AngleXU=\"" <<  AngleXU <<
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
    double AngleXU=0;
    // read my Element
    reader.readElement("ArcOfCircle");
    // get the value of my Attribute
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    CenterZ = reader.getAttributeAsFloat("CenterZ");
    NormalX = reader.getAttributeAsFloat("NormalX");
    NormalY = reader.getAttributeAsFloat("NormalY");
    NormalZ = reader.getAttributeAsFloat("NormalZ");
    if (reader.hasAttribute("AngleXU"))
        AngleXU = reader.getAttributeAsFloat("AngleXU");
    Radius = reader.getAttributeAsFloat("Radius");
    StartAngle = reader.getAttributeAsFloat("StartAngle");
    EndAngle = reader.getAttributeAsFloat("EndAngle");

    // set the read geometry
    gp_Pnt p1(CenterX,CenterY,CenterZ);
    gp_Dir norm(NormalX,NormalY,NormalZ);

    gp_Ax1 normaxis(p1,norm);
    gp_Ax2 xdir(p1, norm);
    xdir.Rotate(normaxis,AngleXU);

    try {
        GC_MakeCircle mc(xdir, Radius);
        if (!mc.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(mc.Status()))
        GC_MakeArcOfCircle ma(mc.Value()->Circ(), StartAngle, EndAngle, 1);
        if (!ma.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(ma.Status()))

        Handle(Geom_TrimmedCurve) tmpcurve = ma.Value();
        Handle(Geom_Circle) tmpcircle = Handle(Geom_Circle)::DownCast(tmpcurve->BasisCurve());
        Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(this->myCurve->BasisCurve());

        circle->SetCirc(tmpcircle->Circ());
        this->myCurve->SetTrim(tmpcurve->FirstParameter(), tmpcurve->LastParameter());
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

PyObject *GeomArcOfCircle::getPyObject(void)
{
    return new ArcOfCirclePy(static_cast<GeomArcOfCircle*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomEllipse,Part::GeomConic)

GeomEllipse::GeomEllipse()
{
    Handle(Geom_Ellipse) e = new Geom_Ellipse(gp_Elips());
    this->myCurve = e;
}

GeomEllipse::GeomEllipse(const Handle(Geom_Ellipse)& e)
{
    setHandle(e);
}

GeomEllipse::~GeomEllipse()
{
}

const Handle(Geom_Geometry)& GeomEllipse::handle() const
{
    return myCurve;
}

void GeomEllipse::setHandle(const Handle(Geom_Ellipse) &e)
{
    this->myCurve = Handle(Geom_Ellipse)::DownCast(e->Copy());
}

Geometry *GeomEllipse::copy(void) const
{
    GeomEllipse *newEllipse = new GeomEllipse(myCurve);
    newEllipse->Construction = this->Construction;
    return newEllipse;
}

GeomBSplineCurve* GeomEllipse::toNurbs(double first, double last) const
{
    // for an arc of ellipse use the generic method
    if (first != 0 || last != 2*M_PI) {
        return GeomCurve::toNurbs(first, last);
    }

    Handle(Geom_Ellipse) conic =  Handle(Geom_Ellipse)::DownCast(handle());
    gp_Ax1 axis = conic->Axis();
    Standard_Real majorRadius = conic->MajorRadius();
    Standard_Real minorRadius = conic->MinorRadius();
    gp_Dir xdir = conic->XAxis().Direction();
    Standard_Real angle = atan2(xdir.Y(), xdir.X());
    const gp_Pnt& loc = axis.Location();

    TColgp_Array1OfPnt poles(1, 7);
    poles(1) = loc.Translated(gp_Vec(majorRadius, 0, 0));
    poles(2) = loc.Translated(gp_Vec(majorRadius, 2*minorRadius, 0));
    poles(3) = loc.Translated(gp_Vec(-majorRadius, 2*minorRadius, 0));
    poles(4) = loc.Translated(gp_Vec(-majorRadius, 0, 0));
    poles(5) = loc.Translated(gp_Vec(-majorRadius, -2*minorRadius, 0));
    poles(6) = loc.Translated(gp_Vec(majorRadius, -2*minorRadius, 0));
    poles(7) = loc.Translated(gp_Vec(majorRadius, 0, 0));

    TColStd_Array1OfReal weights(1,7);
    for (int i=1; i<=7; i++) {
        poles(i).Rotate(axis, angle);
        weights(i) = 1;
    }
    weights(1) = 3;
    weights(4) = 3;
    weights(7) = 3;

    TColStd_Array1OfInteger mults(1, 3);
    mults(1) = 4;
    mults(2) = 3;
    mults(3) = 4;

    TColStd_Array1OfReal knots(1, 3);
    knots(1) = 0;
    knots(2) = 1;
    knots(3) = 2;

    Handle(Geom_BSplineCurve) spline = new Geom_BSplineCurve(poles, weights,knots, mults, 3,
        Standard_False, Standard_True);
    return new GeomBSplineCurve(spline);
}

double GeomEllipse::getMajorRadius(void) const
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(handle());
    return ellipse->MajorRadius();
}

void GeomEllipse::setMajorRadius(double Radius)
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(handle());

    try {
        ellipse->SetMajorRadius(Radius);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

double GeomEllipse::getMinorRadius(void) const
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(handle());
    return ellipse->MinorRadius();
}

void GeomEllipse::setMinorRadius(double Radius)
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(handle());

    try {
        ellipse->SetMinorRadius(Radius);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

/*!
 * \brief GeomEllipse::getMajorAxisDir
 * \return the direction vector (unit-length) of major axis of the ellipse. The
 * direction also points to the first focus.
 */
Base::Vector3d GeomEllipse::getMajorAxisDir() const
{
    gp_Dir xdir = myCurve->XAxis().Direction();
    return Base::Vector3d(xdir.X(), xdir.Y(), xdir.Z());
}

/*!
 * \brief GeomEllipse::setMajorAxisDir Rotates the ellipse in its plane, so
 * that its major axis is as close as possible to the provided direction.
 * \param newdir [in] is the new direction. If the vector is small, the
 * orientation of the ellipse will be preserved. If the vector is not small,
 * but its projection onto plane of the ellipse is small, an exception will be
 * thrown.
 */
void GeomEllipse::setMajorAxisDir(Base::Vector3d newdir)
{
#if OCC_VERSION_HEX >= 0x060504
    if (newdir.Sqr() < Precision::SquareConfusion())
#else
    if (newdir.Length() < Precision::Confusion())
#endif
        return;//zero vector was passed. Keep the old orientation.
    try {
        gp_Ax2 pos = myCurve->Position();
        pos.SetXDirection(gp_Dir(newdir.x, newdir.y, newdir.z));//OCC should keep the old main Direction (Z), and change YDirection to accommodate the new XDirection.
        myCurve->SetPosition(pos);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
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
    gp_Dir xdir = this->myCurve->XAxis().Direction();

    gp_Ax2 xdirref(center, normal); // this is a reference XY for the ellipse

    double AngleXU = -xdir.AngleWithRef(xdirref.XDirection(),normal);


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
            << "AngleXU=\"" << AngleXU << "\" "
            << "/>" << endl;
}

void GeomEllipse::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    GeomCurve::Restore(reader);

    double CenterX,CenterY,CenterZ,NormalX,NormalY,NormalZ,MajorRadius,MinorRadius,AngleXU;
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

    // This is for backwards compatibility
    if(reader.hasAttribute("AngleXU"))
        AngleXU = reader.getAttributeAsFloat("AngleXU");
    else
        AngleXU = 0;

    // set the read geometry
    gp_Pnt p1(CenterX,CenterY,CenterZ);
    gp_Dir norm(NormalX,NormalY,NormalZ);

    gp_Ax1 normaxis(p1,norm);

    gp_Ax2 xdir(p1, norm);

    xdir.Rotate(normaxis,AngleXU);

    try {
        GC_MakeEllipse mc(xdir, MajorRadius, MinorRadius);
        if (!mc.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(mc.Status()))

        this->myCurve = mc.Value();
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

PyObject *GeomEllipse::getPyObject(void)
{
    return new EllipsePy((GeomEllipse*)this->clone());
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomArcOfEllipse,Part::GeomArcOfConic)

GeomArcOfEllipse::GeomArcOfEllipse()
{
    Handle(Geom_Ellipse) e = new Geom_Ellipse(gp_Elips());
    this->myCurve = new Geom_TrimmedCurve(e, e->FirstParameter(),e->LastParameter());
}

GeomArcOfEllipse::GeomArcOfEllipse(const Handle(Geom_Ellipse)& e)
{
    setHandle(e);
}

GeomArcOfEllipse::~GeomArcOfEllipse()
{
}

void GeomArcOfEllipse::setHandle(const Handle(Geom_TrimmedCurve)& c)
{
    Handle(Geom_Ellipse) basis = Handle(Geom_Ellipse)::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not an ellipse");
    this->myCurve = Handle(Geom_TrimmedCurve)::DownCast(c->Copy());
}

void GeomArcOfEllipse::setHandle(const Handle(Geom_Ellipse)& e)
{
    this->myCurve = new Geom_TrimmedCurve(e, e->FirstParameter(),e->LastParameter());
}

const Handle(Geom_Geometry)& GeomArcOfEllipse::handle() const
{
    return myCurve;
}

Geometry *GeomArcOfEllipse::copy(void) const
{
    GeomArcOfEllipse* copy = new GeomArcOfEllipse();
    copy->setHandle(this->myCurve);
    copy->Construction = this->Construction;
    return copy;
}

GeomBSplineCurve* GeomArcOfEllipse::toNurbs(double first, double last) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(curve->BasisCurve());
    return GeomEllipse(ellipse).toNurbs(first, last);
}

double GeomArcOfEllipse::getMajorRadius(void) const
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(myCurve->BasisCurve());
    return ellipse->MajorRadius();
}

void GeomArcOfEllipse::setMajorRadius(double Radius)
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(myCurve->BasisCurve());

    try {
        ellipse->SetMajorRadius(Radius);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

double GeomArcOfEllipse::getMinorRadius(void) const
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(myCurve->BasisCurve());
    return ellipse->MinorRadius();
}

void GeomArcOfEllipse::setMinorRadius(double Radius)
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(myCurve->BasisCurve());

    try {
        ellipse->SetMinorRadius(Radius);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

/*!
 * \brief GeomArcOfEllipse::getMajorAxisDir
 * \return the direction vector (unit-length) of major axis of the ellipse. The
 * direction also points to the first focus.
 */
Base::Vector3d GeomArcOfEllipse::getMajorAxisDir() const
{
    Handle(Geom_Ellipse) c = Handle(Geom_Ellipse)::DownCast( myCurve->BasisCurve() );
    assert(!c.IsNull());
    gp_Dir xdir = c->XAxis().Direction();
    return Base::Vector3d(xdir.X(), xdir.Y(), xdir.Z());
}

/*!
 * \brief GeomArcOfEllipse::setMajorAxisDir Rotates the ellipse in its plane, so
 * that its major axis is as close as possible to the provided direction.
 * \param newdir [in] is the new direction. If the vector is small, the
 * orientation of the ellipse will be preserved. If the vector is not small,
 * but its projection onto plane of the ellipse is small, an exception will be
 * thrown.
 */
void GeomArcOfEllipse::setMajorAxisDir(Base::Vector3d newdir)
{
    Handle(Geom_Ellipse) c = Handle(Geom_Ellipse)::DownCast( myCurve->BasisCurve() );
    assert(!c.IsNull());
#if OCC_VERSION_HEX >= 0x060504
    if (newdir.Sqr() < Precision::SquareConfusion())
#else
    if (newdir.Length() < Precision::Confusion())
#endif
        return;//zero vector was passed. Keep the old orientation.
    try {
        gp_Ax2 pos = c->Position();
        pos.SetXDirection(gp_Dir(newdir.x, newdir.y, newdir.z));//OCC should keep the old main Direction (Z), and change YDirection to accommodate the new XDirection.
        c->SetPosition(pos);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

/*!
 * \brief GeomArcOfEllipse::getRange
 * \param u [out] start angle of the arc, in radians.
 * \param v [out] end angle of the arc, in radians.
 * \param emulateCCWXY: if true, the arc will pretent to be a CCW arc in XY plane.
 * For this to work, the arc must lie in XY plane (i.e. Axis is either +Z or -Z).
 */
void GeomArcOfEllipse::getRange(double& u, double& v, bool emulateCCWXY) const
{
    u = myCurve->FirstParameter();
    v = myCurve->LastParameter();
    if (emulateCCWXY) {
        if (isReversed()) {
            std::swap(u,v);
            u = -u; v = -v;
            if (v < u)
                v += 2*M_PI;
            if (v-u > 2*M_PI)
                v -= 2*M_PI;
        }
    }
}

/*!
 * \brief GeomArcOfEllipse::setRange
 * \param u [in] start angle of the arc, in radians.
 * \param v [in] end angle of the arc, in radians.
 * \param emulateCCWXY: if true, the arc will pretent to be a CCW arc in XY plane.
 * For this to work, the arc must lie in XY plane (i.e. Axis is either +Z or -Z).
 */
void GeomArcOfEllipse::setRange(double u, double v, bool emulateCCWXY)
{
    try {
        if (emulateCCWXY) {
            if (isReversed()) {
                std::swap(u,v);
                u = -u; v = -v;
            }
        }
        myCurve->SetTrim(u, v);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

// Persistence implementer
unsigned int GeomArcOfEllipse::getMemSize (void) const
{
    return sizeof(Geom_Ellipse) + 2 *sizeof(double);
}

void GeomArcOfEllipse::Save(Base::Writer &writer) const
{
    // save the attributes of the father class
    GeomCurve::Save(writer);

    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(this->myCurve->BasisCurve());

    gp_Pnt center = ellipse->Axis().Location();
    gp_Dir normal = ellipse->Axis().Direction();
    gp_Dir xdir = ellipse->XAxis().Direction();

    gp_Ax2 xdirref(center, normal); // this is a reference XY for the ellipse

    double AngleXU = -xdir.AngleWithRef(xdirref.XDirection(),normal);


    writer.Stream()
         << writer.ind()
            << "<ArcOfEllipse "
            << "CenterX=\"" <<  center.X() << "\" "
            << "CenterY=\"" <<  center.Y() << "\" "
            << "CenterZ=\"" <<  center.Z() << "\" "
            << "NormalX=\"" <<  normal.X() << "\" "
            << "NormalY=\"" <<  normal.Y() << "\" "
            << "NormalZ=\"" <<  normal.Z() << "\" "
            << "MajorRadius=\"" <<  ellipse->MajorRadius() << "\" "
            << "MinorRadius=\"" <<  ellipse->MinorRadius() << "\" "
            << "AngleXU=\"" << AngleXU << "\" "
            << "StartAngle=\"" <<  this->myCurve->FirstParameter() << "\" "
            << "EndAngle=\"" <<  this->myCurve->LastParameter() << "\" "
            << "/>" << endl;
}

void GeomArcOfEllipse::Restore(Base::XMLReader &reader)
{
    // read the attributes of the father class
    GeomCurve::Restore(reader);

    double CenterX,CenterY,CenterZ,NormalX,NormalY,NormalZ,MajorRadius,MinorRadius,AngleXU,StartAngle,EndAngle;
    // read my Element
    reader.readElement("ArcOfEllipse");
    // get the value of my Attribute
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    CenterZ = reader.getAttributeAsFloat("CenterZ");
    NormalX = reader.getAttributeAsFloat("NormalX");
    NormalY = reader.getAttributeAsFloat("NormalY");
    NormalZ = reader.getAttributeAsFloat("NormalZ");
    MajorRadius = reader.getAttributeAsFloat("MajorRadius");
    MinorRadius = reader.getAttributeAsFloat("MinorRadius");
    AngleXU = reader.getAttributeAsFloat("AngleXU");
    StartAngle = reader.getAttributeAsFloat("StartAngle");
    EndAngle = reader.getAttributeAsFloat("EndAngle");


    // set the read geometry
    gp_Pnt p1(CenterX,CenterY,CenterZ);
    gp_Dir norm(NormalX,NormalY,NormalZ);

    gp_Ax1 normaxis(p1,norm);

    gp_Ax2 xdir(p1, norm);

    xdir.Rotate(normaxis,AngleXU);

    try {
        GC_MakeEllipse mc(xdir, MajorRadius, MinorRadius);
        if (!mc.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(mc.Status()))

        GC_MakeArcOfEllipse ma(mc.Value()->Elips(), StartAngle, EndAngle, 1);
        if (!ma.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(ma.Status()))

        Handle(Geom_TrimmedCurve) tmpcurve = ma.Value();
        Handle(Geom_Ellipse) tmpellipse = Handle(Geom_Ellipse)::DownCast(tmpcurve->BasisCurve());
        Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(this->myCurve->BasisCurve());

        ellipse->SetElips(tmpellipse->Elips());
        this->myCurve->SetTrim(tmpcurve->FirstParameter(), tmpcurve->LastParameter());
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

PyObject *GeomArcOfEllipse::getPyObject(void)
{
    return new ArcOfEllipsePy(static_cast<GeomArcOfEllipse*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomHyperbola,Part::GeomConic)

GeomHyperbola::GeomHyperbola()
{
    Handle(Geom_Hyperbola) h = new Geom_Hyperbola(gp_Hypr());
    this->myCurve = h;
}

GeomHyperbola::GeomHyperbola(const Handle(Geom_Hyperbola)& h)
{
    setHandle(h);
}

GeomHyperbola::~GeomHyperbola()
{
}

const Handle(Geom_Geometry)& GeomHyperbola::handle() const
{
    return myCurve;
}


void GeomHyperbola::setHandle(const Handle(Geom_Hyperbola)& c)
{
    myCurve = Handle(Geom_Hyperbola)::DownCast(c->Copy());
}

Geometry *GeomHyperbola::copy(void) const
{
    GeomHyperbola *newHyp = new GeomHyperbola(myCurve);
    newHyp->Construction = this->Construction;
    return newHyp;
}

GeomBSplineCurve* GeomHyperbola::toNurbs(double first, double last) const
{
    return GeomCurve::toNurbs(first, last);
}

double GeomHyperbola::getMajorRadius(void) const
{
    Handle(Geom_Hyperbola) h = Handle(Geom_Hyperbola)::DownCast(handle());
    return h->MajorRadius();
}

void GeomHyperbola::setMajorRadius(double Radius)
{
    Handle(Geom_Hyperbola) h = Handle(Geom_Hyperbola)::DownCast(handle());

    try {
        h->SetMajorRadius(Radius);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

double GeomHyperbola::getMinorRadius(void) const
{
    Handle(Geom_Hyperbola) h = Handle(Geom_Hyperbola)::DownCast(handle());
    return h->MinorRadius();
}

void GeomHyperbola::setMinorRadius(double Radius)
{
    Handle(Geom_Hyperbola) h = Handle(Geom_Hyperbola)::DownCast(handle());

    try {
        h->SetMinorRadius(Radius);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

// Persistence implementer
unsigned int GeomHyperbola::getMemSize (void) const
{
    return sizeof(Geom_Hyperbola);
}

void GeomHyperbola::Save(Base::Writer& writer) const
{
    // save the attributes of the father class
    GeomCurve::Save(writer);

    gp_Pnt center = this->myCurve->Axis().Location();
    gp_Dir normal = this->myCurve->Axis().Direction();
    gp_Dir xdir = this->myCurve->XAxis().Direction();

    gp_Ax2 xdirref(center, normal); // this is a reference XY for the ellipse

    double AngleXU = -xdir.AngleWithRef(xdirref.XDirection(),normal);

    writer.Stream()
         << writer.ind()
            << "<Hyperbola "
            << "CenterX=\"" <<  center.X() << "\" "
            << "CenterY=\"" <<  center.Y() << "\" "
            << "CenterZ=\"" <<  center.Z() << "\" "
            << "NormalX=\"" <<  normal.X() << "\" "
            << "NormalY=\"" <<  normal.Y() << "\" "
            << "NormalZ=\"" <<  normal.Z() << "\" "
            << "MajorRadius=\"" <<  this->myCurve->MajorRadius() << "\" "
            << "MinorRadius=\"" <<  this->myCurve->MinorRadius() << "\" "
            << "AngleXU=\"" << AngleXU << "\" "
            << "/>" << endl;
}

void GeomHyperbola::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    GeomCurve::Restore(reader);

    double CenterX,CenterY,CenterZ,NormalX,NormalY,NormalZ,MajorRadius,MinorRadius,AngleXU;
    // read my Element
    reader.readElement("Hyperbola");
    // get the value of my Attribute
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    CenterZ = reader.getAttributeAsFloat("CenterZ");
    NormalX = reader.getAttributeAsFloat("NormalX");
    NormalY = reader.getAttributeAsFloat("NormalY");
    NormalZ = reader.getAttributeAsFloat("NormalZ");
    MajorRadius = reader.getAttributeAsFloat("MajorRadius");
    MinorRadius = reader.getAttributeAsFloat("MinorRadius");
    AngleXU = reader.getAttributeAsFloat("AngleXU");

    // set the read geometry
    gp_Pnt p1(CenterX,CenterY,CenterZ);
    gp_Dir norm(NormalX,NormalY,NormalZ);

    gp_Ax1 normaxis(p1,norm);

    gp_Ax2 xdir(p1, norm);

    xdir.Rotate(normaxis,AngleXU);

    try {
        GC_MakeHyperbola mc(xdir, MajorRadius, MinorRadius);
        if (!mc.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(mc.Status()))

        this->myCurve = mc.Value();
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

PyObject *GeomHyperbola::getPyObject(void)
{
    return new HyperbolaPy(static_cast<GeomHyperbola*>(this->clone()));
}
// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomArcOfHyperbola,Part::GeomArcOfConic)

GeomArcOfHyperbola::GeomArcOfHyperbola()
{
    gp_Ax2 ax2 = gp_Ax2();
    Handle(Geom_Hyperbola) h = new Geom_Hyperbola(gp_Hypr(ax2, 1,1));
    this->myCurve = new Geom_TrimmedCurve(h, h->FirstParameter(),h->LastParameter());
}

GeomArcOfHyperbola::GeomArcOfHyperbola(const Handle(Geom_Hyperbola)& h)
{
    setHandle(h);
}

GeomArcOfHyperbola::~GeomArcOfHyperbola()
{
}

void GeomArcOfHyperbola::setHandle(const Handle(Geom_TrimmedCurve)& c)
{
    Handle(Geom_Hyperbola) basis = Handle(Geom_Hyperbola)::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not an hyperbola");
    this->myCurve = Handle(Geom_TrimmedCurve)::DownCast(c->Copy());
}

void GeomArcOfHyperbola::setHandle(const Handle(Geom_Hyperbola)& h)
{
    this->myCurve = new Geom_TrimmedCurve(h, h->FirstParameter(),h->LastParameter());
}

const Handle(Geom_Geometry)& GeomArcOfHyperbola::handle() const
{
    return myCurve;
}

Geometry *GeomArcOfHyperbola::copy(void) const
{
    GeomArcOfHyperbola* copy = new GeomArcOfHyperbola();
    copy->setHandle(this->myCurve);
    copy->Construction = this->Construction;
    return copy;
}

GeomBSplineCurve* GeomArcOfHyperbola::toNurbs(double first, double last) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(curve->BasisCurve());
    return GeomHyperbola(hyperbola).toNurbs(first, last);
}

double GeomArcOfHyperbola::getMajorRadius(void) const
{
    Handle(Geom_Hyperbola) h = Handle(Geom_Hyperbola)::DownCast(myCurve->BasisCurve());
    return h->MajorRadius();
}

void GeomArcOfHyperbola::setMajorRadius(double Radius)
{
    Handle(Geom_Hyperbola) h = Handle(Geom_Hyperbola)::DownCast(myCurve->BasisCurve());

    try {
        h->SetMajorRadius(Radius);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

double GeomArcOfHyperbola::getMinorRadius(void) const
{
    Handle(Geom_Hyperbola) h = Handle(Geom_Hyperbola)::DownCast(myCurve->BasisCurve());
    return h->MinorRadius();
}

void GeomArcOfHyperbola::setMinorRadius(double Radius)
{
    Handle(Geom_Hyperbola) h = Handle(Geom_Hyperbola)::DownCast(myCurve->BasisCurve());

    try {
        h->SetMinorRadius(Radius);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

/*!
 * \brief GeomArcOfHyperbola::getMajorAxisDir
 * \return the direction vector (unit-length) of major axis of the hyperbola. The
 * direction also points to the first focus.
 */
Base::Vector3d GeomArcOfHyperbola::getMajorAxisDir() const
{
    Handle(Geom_Hyperbola) c = Handle(Geom_Hyperbola)::DownCast( myCurve->BasisCurve() );
    assert(!c.IsNull());
    gp_Dir xdir = c->XAxis().Direction();
    return Base::Vector3d(xdir.X(), xdir.Y(), xdir.Z());
}

/*!
 * \brief GeomArcOfHyperbola::setMajorAxisDir Rotates the hyperbola in its plane, so
 * that its major axis is as close as possible to the provided direction.
 * \param newdir [in] is the new direction. If the vector is small, the
 * orientation of the ellipse will be preserved. If the vector is not small,
 * but its projection onto plane of the ellipse is small, an exception will be
 * thrown.
 */
void GeomArcOfHyperbola::setMajorAxisDir(Base::Vector3d newdir)
{
    Handle(Geom_Hyperbola) c = Handle(Geom_Hyperbola)::DownCast( myCurve->BasisCurve() );
    assert(!c.IsNull());
    #if OCC_VERSION_HEX >= 0x060504
    if (newdir.Sqr() < Precision::SquareConfusion())
    #else
    if (newdir.Length() < Precision::Confusion())
    #endif
        return;//zero vector was passed. Keep the old orientation.

    try {
        gp_Ax2 pos = c->Position();
        pos.SetXDirection(gp_Dir(newdir.x, newdir.y, newdir.z));//OCC should keep the old main Direction (Z), and change YDirection to accommodate the new XDirection.
        c->SetPosition(pos);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

void GeomArcOfHyperbola::getRange(double& u, double& v, bool emulateCCWXY) const
{
    try {
        if (emulateCCWXY){
            if (isReversed()) {
                Handle(Geom_Hyperbola) c = Handle(Geom_Hyperbola)::DownCast(myCurve->BasisCurve());
                assert(!c.IsNull());
                c->Reverse();
            }
        }
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }

    u = myCurve->FirstParameter();
    v = myCurve->LastParameter();
}

void GeomArcOfHyperbola::setRange(double u, double v, bool emulateCCWXY)
{
    try {
        myCurve->SetTrim(u, v);

        if (emulateCCWXY) {
            if (isReversed()) {
                Handle(Geom_Hyperbola) c = Handle(Geom_Hyperbola)::DownCast(myCurve->BasisCurve());
                assert(!c.IsNull());
                c->Reverse();
            }
        }
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

// Persistence implementer
unsigned int GeomArcOfHyperbola::getMemSize (void) const
{
    return sizeof(Geom_Hyperbola) + 2 *sizeof(double);
}

void GeomArcOfHyperbola::Save(Base::Writer &writer) const
{
    // save the attributes of the father class
    GeomCurve::Save(writer);

    Handle(Geom_Hyperbola) h = Handle(Geom_Hyperbola)::DownCast(this->myCurve->BasisCurve());

    gp_Pnt center = h->Axis().Location();
    gp_Dir normal = h->Axis().Direction();
    gp_Dir xdir = h->XAxis().Direction();

    gp_Ax2 xdirref(center, normal); // this is a reference XY for the ellipse

    double AngleXU = -xdir.AngleWithRef(xdirref.XDirection(),normal);

    writer.Stream()
         << writer.ind()
            << "<ArcOfHyperbola "
            << "CenterX=\"" <<  center.X() << "\" "
            << "CenterY=\"" <<  center.Y() << "\" "
            << "CenterZ=\"" <<  center.Z() << "\" "
            << "NormalX=\"" <<  normal.X() << "\" "
            << "NormalY=\"" <<  normal.Y() << "\" "
            << "NormalZ=\"" <<  normal.Z() << "\" "
            << "MajorRadius=\"" <<  h->MajorRadius() << "\" "
            << "MinorRadius=\"" <<  h->MinorRadius() << "\" "
            << "AngleXU=\"" << AngleXU << "\" "
            << "StartAngle=\"" <<  this->myCurve->FirstParameter() << "\" "
            << "EndAngle=\"" <<  this->myCurve->LastParameter() << "\" "
            << "/>" << endl;
}

void GeomArcOfHyperbola::Restore(Base::XMLReader &reader)
{
    // read the attributes of the father class
    GeomCurve::Restore(reader);

    double CenterX,CenterY,CenterZ,NormalX,NormalY,NormalZ,MajorRadius,MinorRadius,AngleXU,StartAngle,EndAngle;
    // read my Element
    reader.readElement("ArcOfHyperbola");
    // get the value of my Attribute
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    CenterZ = reader.getAttributeAsFloat("CenterZ");
    NormalX = reader.getAttributeAsFloat("NormalX");
    NormalY = reader.getAttributeAsFloat("NormalY");
    NormalZ = reader.getAttributeAsFloat("NormalZ");
    MajorRadius = reader.getAttributeAsFloat("MajorRadius");
    MinorRadius = reader.getAttributeAsFloat("MinorRadius");
    AngleXU = reader.getAttributeAsFloat("AngleXU");
    StartAngle = reader.getAttributeAsFloat("StartAngle");
    EndAngle = reader.getAttributeAsFloat("EndAngle");


    // set the read geometry
    gp_Pnt p1(CenterX,CenterY,CenterZ);
    gp_Dir norm(NormalX,NormalY,NormalZ);

    gp_Ax1 normaxis(p1,norm);

    gp_Ax2 xdir(p1, norm);

    xdir.Rotate(normaxis,AngleXU);

    try {
        GC_MakeHyperbola mc(xdir, MajorRadius, MinorRadius);
        if (!mc.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(mc.Status()))

        GC_MakeArcOfHyperbola ma(mc.Value()->Hypr(), StartAngle, EndAngle, 1);
        if (!ma.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(ma.Status()))

        Handle(Geom_TrimmedCurve) tmpcurve = ma.Value();
        Handle(Geom_Hyperbola) tmphyperbola = Handle(Geom_Hyperbola)::DownCast(tmpcurve->BasisCurve());
        Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(this->myCurve->BasisCurve());

        hyperbola->SetHypr(tmphyperbola->Hypr());
        this->myCurve->SetTrim(tmpcurve->FirstParameter(), tmpcurve->LastParameter());
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

PyObject *GeomArcOfHyperbola::getPyObject(void)
{
    return new ArcOfHyperbolaPy(static_cast<GeomArcOfHyperbola*>(this->clone()));
}
// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomParabola,Part::GeomConic)

GeomParabola::GeomParabola()
{
    Handle(Geom_Parabola) p = new Geom_Parabola(gp_Parab());
    this->myCurve = p;
}

GeomParabola::GeomParabola(const Handle(Geom_Parabola)& p)
{
    setHandle(p);
}

GeomParabola::~GeomParabola()
{
}

const Handle(Geom_Geometry)& GeomParabola::handle() const
{
    return myCurve;
}

void GeomParabola::setHandle(const Handle(Geom_Parabola)& c)
{
    myCurve = Handle(Geom_Parabola)::DownCast(c->Copy());
}

Geometry *GeomParabola::copy(void) const
{
    GeomParabola *newPar = new GeomParabola(myCurve);
    newPar->Construction = this->Construction;
    return newPar;
}

GeomBSplineCurve* GeomParabola::toNurbs(double first, double last) const
{
    // the default implementation suffices because a non-rational B-spline with
    // one segment is a parabola
    return GeomCurve::toNurbs(first, last);
}

double GeomParabola::getFocal(void) const
{
    Handle(Geom_Parabola) p = Handle(Geom_Parabola)::DownCast(handle());
    return p->Focal();
}

void GeomParabola::setFocal(double length)
{
    Handle(Geom_Parabola) p = Handle(Geom_Parabola)::DownCast(handle());

    try {
        p->SetFocal(length);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

// Persistence implementer
unsigned int GeomParabola::getMemSize (void) const
{
    return sizeof(Geom_Parabola);
}

void GeomParabola::Save(Base::Writer& writer) const
{
    // save the attributes of the father class
    GeomCurve::Save(writer);

    gp_Pnt center = this->myCurve->Axis().Location();
    gp_Dir normal = this->myCurve->Axis().Direction();
    gp_Dir xdir = this->myCurve->XAxis().Direction();

    gp_Ax2 xdirref(center, normal); // this is a reference XY for the ellipse

    double AngleXU = -xdir.AngleWithRef(xdirref.XDirection(),normal);

    writer.Stream()
         << writer.ind()
            << "<Parabola "
            << "CenterX=\"" <<  center.X() << "\" "
            << "CenterY=\"" <<  center.Y() << "\" "
            << "CenterZ=\"" <<  center.Z() << "\" "
            << "NormalX=\"" <<  normal.X() << "\" "
            << "NormalY=\"" <<  normal.Y() << "\" "
            << "NormalZ=\"" <<  normal.Z() << "\" "
            << "Focal=\"" <<  this->myCurve->Focal() << "\" "
            << "AngleXU=\"" << AngleXU << "\" "
            << "/>" << endl;
}

void GeomParabola::Restore(Base::XMLReader& reader)
{
    // read the attributes of the father class
    GeomCurve::Restore(reader);

    double CenterX,CenterY,CenterZ,NormalX,NormalY,NormalZ,Focal,AngleXU;
    // read my Element
    reader.readElement("Parabola");
    // get the value of my Attribute
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    CenterZ = reader.getAttributeAsFloat("CenterZ");
    NormalX = reader.getAttributeAsFloat("NormalX");
    NormalY = reader.getAttributeAsFloat("NormalY");
    NormalZ = reader.getAttributeAsFloat("NormalZ");
    Focal = reader.getAttributeAsFloat("Focal");
    AngleXU = reader.getAttributeAsFloat("AngleXU");

    // set the read geometry
    gp_Pnt p1(CenterX,CenterY,CenterZ);
    gp_Dir norm(NormalX,NormalY,NormalZ);

    gp_Ax1 normaxis(p1,norm);

    gp_Ax2 xdir(p1, norm);

    xdir.Rotate(normaxis,AngleXU);

    try {
        gce_MakeParab mc(xdir, Focal);
        if (!mc.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(mc.Status()))

        this->myCurve = new Geom_Parabola(mc.Value());
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

PyObject *GeomParabola::getPyObject(void)
{
    return new ParabolaPy(static_cast<GeomParabola*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomArcOfParabola,Part::GeomArcOfConic)

GeomArcOfParabola::GeomArcOfParabola()
{
    Handle(Geom_Parabola) p = new Geom_Parabola(gp_Parab());
    this->myCurve = new Geom_TrimmedCurve(p, p->FirstParameter(),p->LastParameter());
}

GeomArcOfParabola::GeomArcOfParabola(const Handle(Geom_Parabola)& h)
{
    setHandle(h);
}

GeomArcOfParabola::~GeomArcOfParabola()
{
}

void GeomArcOfParabola::setHandle(const Handle(Geom_TrimmedCurve)& c)
{
    Handle(Geom_Parabola) basis = Handle(Geom_Parabola)::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not a parabola");
    this->myCurve = Handle(Geom_TrimmedCurve)::DownCast(c->Copy());
}

void GeomArcOfParabola::setHandle(const Handle(Geom_Parabola)& h)
{
    this->myCurve = new Geom_TrimmedCurve(h, h->FirstParameter(),h->LastParameter());
}

const Handle(Geom_Geometry)& GeomArcOfParabola::handle() const
{
    return myCurve;
}

Geometry *GeomArcOfParabola::copy(void) const
{
    GeomArcOfParabola* copy = new GeomArcOfParabola();
    copy->setHandle(this->myCurve);
    copy->Construction = this->Construction;
    return copy;
}

GeomBSplineCurve* GeomArcOfParabola::toNurbs(double first, double last) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Parabola) parabola = Handle(Geom_Parabola)::DownCast(curve->BasisCurve());
    return GeomParabola(parabola).toNurbs(first, last);
}

double GeomArcOfParabola::getFocal(void) const
{
    Handle(Geom_Parabola) p = Handle(Geom_Parabola)::DownCast(myCurve->BasisCurve());
    return p->Focal();
}

void GeomArcOfParabola::setFocal(double length)
{
    Handle(Geom_Parabola) p = Handle(Geom_Parabola)::DownCast(myCurve->BasisCurve());

    try {
        p->SetFocal(length);
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

Base::Vector3d GeomArcOfParabola::getFocus(void) const
{
    Handle(Geom_Parabola) p = Handle(Geom_Parabola)::DownCast(myCurve->BasisCurve());
    gp_Pnt gp = p->Focus();

    return Base::Vector3d(gp.X(),gp.Y(),gp.Z());
}

void GeomArcOfParabola::getRange(double& u, double& v, bool emulateCCWXY) const
{
    try {
        if (emulateCCWXY) {
            if (isReversed()) {
                Handle(Geom_Parabola) c = Handle(Geom_Parabola)::DownCast(myCurve->BasisCurve());
                assert(!c.IsNull());
                c->Reverse();
            }
        }
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }

    u = myCurve->FirstParameter();
    v = myCurve->LastParameter();
}

void GeomArcOfParabola::setRange(double u, double v, bool emulateCCWXY)
{
    try {
        myCurve->SetTrim(u, v);
        if (emulateCCWXY) {
            if (isReversed()) {
                Handle(Geom_Parabola) c = Handle(Geom_Parabola)::DownCast(myCurve->BasisCurve());
                assert(!c.IsNull());
                c->Reverse();
            }
        }
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

// Persistence implementer
unsigned int GeomArcOfParabola::getMemSize (void) const
{
    return sizeof(Geom_Parabola) + 2 *sizeof(double);
}

void GeomArcOfParabola::Save(Base::Writer &writer) const
{
    // save the attributes of the father class
    GeomCurve::Save(writer);

    Handle(Geom_Parabola) p = Handle(Geom_Parabola)::DownCast(this->myCurve->BasisCurve());

    gp_Pnt center = p->Axis().Location();
    gp_Dir normal = p->Axis().Direction();
    gp_Dir xdir = p->XAxis().Direction();

    gp_Ax2 xdirref(center, normal); // this is a reference XY for the ellipse

    double AngleXU = -xdir.AngleWithRef(xdirref.XDirection(),normal);

    writer.Stream()
         << writer.ind()
            << "<ArcOfParabola "
            << "CenterX=\"" <<  center.X() << "\" "
            << "CenterY=\"" <<  center.Y() << "\" "
            << "CenterZ=\"" <<  center.Z() << "\" "
            << "NormalX=\"" <<  normal.X() << "\" "
            << "NormalY=\"" <<  normal.Y() << "\" "
            << "NormalZ=\"" <<  normal.Z() << "\" "
            << "Focal=\"" <<  p->Focal() << "\" "
            << "AngleXU=\"" << AngleXU << "\" "
            << "StartAngle=\"" <<  this->myCurve->FirstParameter() << "\" "
            << "EndAngle=\"" <<  this->myCurve->LastParameter() << "\" "
            << "/>" << endl;
}

void GeomArcOfParabola::Restore(Base::XMLReader &reader)
{
    // read the attributes of the father class
    GeomCurve::Restore(reader);

    double CenterX,CenterY,CenterZ,NormalX,NormalY,NormalZ,Focal,AngleXU,StartAngle,EndAngle;
    // read my Element
    reader.readElement("ArcOfParabola");
    // get the value of my Attribute
    CenterX = reader.getAttributeAsFloat("CenterX");
    CenterY = reader.getAttributeAsFloat("CenterY");
    CenterZ = reader.getAttributeAsFloat("CenterZ");
    NormalX = reader.getAttributeAsFloat("NormalX");
    NormalY = reader.getAttributeAsFloat("NormalY");
    NormalZ = reader.getAttributeAsFloat("NormalZ");
    Focal = reader.getAttributeAsFloat("Focal");
    AngleXU = reader.getAttributeAsFloat("AngleXU");
    StartAngle = reader.getAttributeAsFloat("StartAngle");
    EndAngle = reader.getAttributeAsFloat("EndAngle");


    // set the read geometry
    gp_Pnt p1(CenterX,CenterY,CenterZ);
    gp_Dir norm(NormalX,NormalY,NormalZ);

    gp_Ax1 normaxis(p1,norm);

    gp_Ax2 xdir(p1, norm);

    xdir.Rotate(normaxis,AngleXU);

    try {
        gce_MakeParab mc(xdir, Focal);
        if (!mc.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(mc.Status()))

        GC_MakeArcOfParabola ma(mc.Value(), StartAngle, EndAngle, 1);
        if (!ma.IsDone())
            THROWM(Base::CADKernelError,gce_ErrorStatusText(ma.Status()))

        Handle(Geom_TrimmedCurve) tmpcurve = ma.Value();
        Handle(Geom_Parabola) tmpparabola = Handle(Geom_Parabola)::DownCast(tmpcurve->BasisCurve());
        Handle(Geom_Parabola) parabola = Handle(Geom_Parabola)::DownCast(this->myCurve->BasisCurve());

        parabola->SetParab(tmpparabola->Parab());
        this->myCurve->SetTrim(tmpcurve->FirstParameter(), tmpcurve->LastParameter());
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

PyObject *GeomArcOfParabola::getPyObject(void)
{
    return new ArcOfParabolaPy(static_cast<GeomArcOfParabola*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomLine,Part::GeomCurve)

GeomLine::GeomLine()
{
    Handle(Geom_Line) c = new Geom_Line(gp_Lin());
    this->myCurve = c;
}

GeomLine::GeomLine(const Handle(Geom_Line)& l)
{
    setHandle(l);
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

const Handle(Geom_Geometry)& GeomLine::handle() const
{
    return myCurve;
}


void GeomLine::setHandle(const Handle(Geom_Line)& l)
{
    this->myCurve = Handle(Geom_Line)::DownCast(l->Copy());
}

Geometry *GeomLine::copy(void) const
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

TYPESYSTEM_SOURCE(Part::GeomLineSegment,Part::GeomTrimmedCurve)

GeomLineSegment::GeomLineSegment()
{
    gp_Lin line;
    line.SetLocation(gp_Pnt(0.0,0.0,0.0));
    line.SetDirection(gp_Dir(0.0,0.0,1.0));
    Handle(Geom_Line) c = new Geom_Line(line);
    this->myCurve = new Geom_TrimmedCurve(c, 0.0,1.0);
}

GeomLineSegment::GeomLineSegment(const Handle(Geom_Line)& l)
{
    setHandle(l);
}

GeomLineSegment::~GeomLineSegment()
{
}

void GeomLineSegment::setHandle(const Handle(Geom_TrimmedCurve)& c)
{
    Handle(Geom_Line) basis = Handle(Geom_Line)::DownCast(c->BasisCurve());
    if (basis.IsNull())
        Standard_Failure::Raise("Basis curve is not a line");
    this->myCurve = Handle(Geom_TrimmedCurve)::DownCast(c->Copy());
}

void GeomLineSegment::setHandle(const Handle(Geom_Line)& l)
{
    this->myCurve = new Geom_TrimmedCurve(l, l->FirstParameter(),l->LastParameter());
}

const Handle(Geom_Geometry)& GeomLineSegment::handle() const
{
    return myCurve;
}

Geometry *GeomLineSegment::copy(void)const
{
    GeomLineSegment *tempCurve = new GeomLineSegment();
    tempCurve->myCurve = Handle(Geom_TrimmedCurve)::DownCast(myCurve->Copy());
    tempCurve->Construction = this->Construction;
    return tempCurve;
}

Base::Vector3d GeomLineSegment::getStartPoint() const
{
    Handle(Geom_TrimmedCurve) this_curve = Handle(Geom_TrimmedCurve)::DownCast(handle());
    gp_Pnt pnt = this_curve->StartPoint();
    return Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
}

Base::Vector3d GeomLineSegment::getEndPoint() const
{
    Handle(Geom_TrimmedCurve) this_curve = Handle(Geom_TrimmedCurve)::DownCast(handle());
    gp_Pnt pnt = this_curve->EndPoint();
    return Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
}

void GeomLineSegment::setPoints(const Base::Vector3d& Start, const Base::Vector3d& End)
{
    gp_Pnt p1(Start.x,Start.y,Start.z), p2(End.x,End.y,End.z);
    Handle(Geom_TrimmedCurve) this_curv = Handle(Geom_TrimmedCurve)::DownCast(handle());

    try {
        // Create line out of two points
        if (p1.Distance(p2) < gp::Resolution())
            THROWM(Base::ValueError,"Both points are equal");

        GC_MakeSegment ms(p1, p2);
        if (!ms.IsDone()) {
            THROWM(Base::CADKernelError,gce_ErrorStatusText(ms.Status()))
        }

        // get Geom_Line of line segment
        Handle(Geom_Line) this_line = Handle(Geom_Line)::DownCast
            (this_curv->BasisCurve());
        Handle(Geom_TrimmedCurve) that_curv = ms.Value();
        Handle(Geom_Line) that_line = Handle(Geom_Line)::DownCast(that_curv->BasisCurve());
        this_line->SetLin(that_line->Lin());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
    }
    catch (Standard_Failure& e) {

        THROWM(Base::CADKernelError,e.GetMessageString())
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

    Base::Vector3d start(StartX,StartY,StartZ);
    Base::Vector3d end(EndX,EndY,EndZ);
    // set the read geometry
    try {
        setPoints(start, end);
    }
    catch(Base::ValueError&) {
        // for a line segment construction, the only possibility of a value error is that
        // the points are too close. The best try to restore is incrementing the distance.
        // for other objects, the best effort may be just to leave default values.
        reader.setPartialRestore(true);

        if(start.x == 0) {
            end = start + Base::Vector3d(DBL_EPSILON,0,0);
        }
        else {
            end = start + Base::Vector3d(start.x*DBL_EPSILON,0,0);
        }

        setPoints(start, end);
    }
}

PyObject *GeomLineSegment::getPyObject(void)
{
    return new LineSegmentPy(static_cast<GeomLineSegment*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomOffsetCurve,Part::GeomCurve)

GeomOffsetCurve::GeomOffsetCurve()
{
}

GeomOffsetCurve::GeomOffsetCurve(const Handle(Geom_Curve)& c, double offset, const gp_Dir& dir)
{
    this->myCurve = new Geom_OffsetCurve(c, offset, dir);
}

GeomOffsetCurve::GeomOffsetCurve(const Handle(Geom_Curve)& c, double offset, Base::Vector3d& dir):GeomOffsetCurve(c,offset,gp_Dir(dir.x,dir.y,dir.z))
{
}

GeomOffsetCurve::GeomOffsetCurve(const Handle(Geom_OffsetCurve)& c)
{
    setHandle(c);
}

GeomOffsetCurve::~GeomOffsetCurve()
{
}

Geometry *GeomOffsetCurve::copy(void) const
{
    GeomOffsetCurve *newCurve = new GeomOffsetCurve(myCurve);
    newCurve->Construction = this->Construction;
    return newCurve;
}

void GeomOffsetCurve::setHandle(const Handle(Geom_OffsetCurve)& c)
{
    this->myCurve = Handle(Geom_OffsetCurve)::DownCast(c->Copy());
}

const Handle(Geom_Geometry)& GeomOffsetCurve::handle() const
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


TYPESYSTEM_SOURCE_ABSTRACT(Part::GeomSurface,Part::Geometry)

GeomSurface::GeomSurface()
{
}

GeomSurface::~GeomSurface()
{
}

TopoDS_Shape GeomSurface::toShape() const
{
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(handle());
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
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(handle());
    GeomLProp_SLProps prop(s,u,v,2,Precision::Confusion());
    if (prop.IsTangentUDefined()) {
        prop.TangentU(dirU);
        return true;
    }

    return false;
}

bool GeomSurface::tangentV(double u, double v, gp_Dir& dirV) const
{
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(handle());
    GeomLProp_SLProps prop(s,u,v,2,Precision::Confusion());
    if (prop.IsTangentVDefined()) {
        prop.TangentV(dirV);
        return true;
    }

    return false;
}

bool GeomSurface::normal(double u, double v, gp_Dir& dir) const
{
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(handle());
    GeomLProp_SLProps prop(s,u,v,2,Precision::Confusion());
    if (prop.IsNormalDefined()) {
        dir = prop.Normal();
        return true;
    }

    return false;
}

bool GeomSurface::isUmbillic(double u, double v) const
{
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(handle());
    GeomLProp_SLProps prop(s,u,v,2,Precision::Confusion());
    if (prop.IsCurvatureDefined()) {
        return prop.IsUmbilic();
    }

    THROWM(Base::RuntimeError,"No curvature defined")
}

double GeomSurface::curvature(double u, double v, Curvature type) const
{
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(handle());
    GeomLProp_SLProps prop(s,u,v,2,Precision::Confusion());
    if (prop.IsCurvatureDefined()) {
        double value = 0;
        switch (type) {
        case Maximum:
            value = prop.MaxCurvature();
            break;
        case Minimum:
            value = prop.MinCurvature();
            break;
        case Mean:
            value = prop.MeanCurvature();
            break;
        case Gaussian:
            value = prop.GaussianCurvature();
            break;
        }

        return value;
    }

    THROWM(Base::RuntimeError,"No curvature defined")
}

void GeomSurface::curvatureDirections(double u, double v, gp_Dir& maxD, gp_Dir& minD) const
{
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(handle());
    GeomLProp_SLProps prop(s,u,v,2,Precision::Confusion());
    if (prop.IsCurvatureDefined()) {
        prop.CurvatureDirections(maxD, minD);
        return;
    }

    THROWM(Base::RuntimeError,"No curvature defined")
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomBezierSurface,Part::GeomSurface)

GeomBezierSurface::GeomBezierSurface()
{
    TColgp_Array2OfPnt poles(1,2,1,2);
    poles(1,1) = gp_Pnt(0.0,0.0,0.0);
    poles(2,1) = gp_Pnt(1.0,0.0,0.0);
    poles(1,2) = gp_Pnt(0.0,1.0,0.0);
    poles(2,2) = gp_Pnt(1.0,1.0,0.0);
    this->mySurface = new Geom_BezierSurface(poles);
}

GeomBezierSurface::GeomBezierSurface(const Handle(Geom_BezierSurface)& b)
{
    setHandle(b);
}

GeomBezierSurface::~GeomBezierSurface()
{
}

const Handle(Geom_Geometry)& GeomBezierSurface::handle() const
{
    return mySurface;
}

void GeomBezierSurface::setHandle(const Handle(Geom_BezierSurface)& b)
{
    this->mySurface = Handle(Geom_BezierSurface)::DownCast(b->Copy());
}

Geometry *GeomBezierSurface::copy(void) const
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

TYPESYSTEM_SOURCE(Part::GeomBSplineSurface,Part::GeomSurface)

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

GeomBSplineSurface::GeomBSplineSurface(const Handle(Geom_BSplineSurface)& b)
{
    setHandle(b);
}

GeomBSplineSurface::~GeomBSplineSurface()
{
}

void GeomBSplineSurface::setHandle(const Handle(Geom_BSplineSurface)& s)
{
    mySurface = Handle(Geom_BSplineSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomBSplineSurface::handle() const
{
    return mySurface;
}

Geometry *GeomBSplineSurface::copy(void) const
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

TYPESYSTEM_SOURCE(Part::GeomCylinder,Part::GeomSurface)

GeomCylinder::GeomCylinder()
{
    Handle(Geom_CylindricalSurface) s = new Geom_CylindricalSurface(gp_Cylinder());
    this->mySurface = s;
}

GeomCylinder::GeomCylinder(const Handle(Geom_CylindricalSurface)& c)
{
    setHandle(c);
}

GeomCylinder::~GeomCylinder()
{
}

void GeomCylinder::setHandle(const Handle(Geom_CylindricalSurface)& s)
{
    mySurface = Handle(Geom_CylindricalSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomCylinder::handle() const
{
    return mySurface;
}

Geometry *GeomCylinder::copy(void) const
{
    GeomCylinder *tempCurve = new GeomCylinder();
    tempCurve->mySurface = Handle(Geom_CylindricalSurface)::DownCast(mySurface->Copy());
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

TYPESYSTEM_SOURCE(Part::GeomCone,Part::GeomSurface)

GeomCone::GeomCone()
{
    Handle(Geom_ConicalSurface) s = new Geom_ConicalSurface(gp_Cone());
    this->mySurface = s;
}

GeomCone::GeomCone(const Handle(Geom_ConicalSurface)& c)
{
    setHandle(c);
}

GeomCone::~GeomCone()
{
}

void GeomCone::setHandle(const Handle(Geom_ConicalSurface)& s)
{
    mySurface = Handle(Geom_ConicalSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomCone::handle() const
{
    return mySurface;
}

Geometry *GeomCone::copy(void) const
{
    GeomCone *tempCurve = new GeomCone();
    tempCurve->mySurface = Handle(Geom_ConicalSurface)::DownCast(mySurface->Copy());
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

TYPESYSTEM_SOURCE(Part::GeomToroid,Part::GeomSurface)

GeomToroid::GeomToroid()
{
    Handle(Geom_ToroidalSurface) s = new Geom_ToroidalSurface(gp_Torus());
    this->mySurface = s;
}

GeomToroid::GeomToroid(const Handle(Geom_ToroidalSurface)& t)
{
    setHandle(t);
}

GeomToroid::~GeomToroid()
{
}

void GeomToroid::setHandle(const Handle(Geom_ToroidalSurface)& s)
{
    mySurface = Handle(Geom_ToroidalSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomToroid::handle() const
{
    return mySurface;
}

Geometry *GeomToroid::copy(void) const
{
    GeomToroid *tempCurve = new GeomToroid();
    tempCurve->mySurface = Handle(Geom_ToroidalSurface)::DownCast(mySurface->Copy());
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

TYPESYSTEM_SOURCE(Part::GeomSphere,Part::GeomSurface)

GeomSphere::GeomSphere()
{
    Handle(Geom_SphericalSurface) s = new Geom_SphericalSurface(gp_Sphere());
    this->mySurface = s;
}

GeomSphere::GeomSphere(const Handle(Geom_SphericalSurface)& s)
{
    setHandle(s);
}

GeomSphere::~GeomSphere()
{
}

void GeomSphere::setHandle(const Handle(Geom_SphericalSurface)& s)
{
    mySurface = Handle(Geom_SphericalSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomSphere::handle() const
{
    return mySurface;
}

Geometry *GeomSphere::copy(void) const
{
    GeomSphere *tempCurve = new GeomSphere();
    tempCurve->mySurface = Handle(Geom_SphericalSurface)::DownCast(mySurface->Copy());
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

TYPESYSTEM_SOURCE(Part::GeomPlane,Part::GeomSurface)

GeomPlane::GeomPlane()
{
    Handle(Geom_Plane) s = new Geom_Plane(gp_Pln());
    this->mySurface = s;
}

GeomPlane::GeomPlane(const Handle(Geom_Plane)& p)
{
    setHandle(p);
}

GeomPlane::~GeomPlane()
{
}

void GeomPlane::setHandle(const Handle(Geom_Plane)& s)
{
    mySurface = Handle(Geom_Plane)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomPlane::handle() const
{
    return mySurface;
}

Geometry *GeomPlane::copy(void) const
{
    GeomPlane *tempCurve = new GeomPlane();
    tempCurve->mySurface = Handle(Geom_Plane)::DownCast(mySurface->Copy());
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

TYPESYSTEM_SOURCE(Part::GeomOffsetSurface,Part::GeomSurface)

GeomOffsetSurface::GeomOffsetSurface()
{
}

GeomOffsetSurface::GeomOffsetSurface(const Handle(Geom_Surface)& s, double offset)
{
    this->mySurface = new Geom_OffsetSurface(s, offset);
}

GeomOffsetSurface::GeomOffsetSurface(const Handle(Geom_OffsetSurface)& s)
{
    setHandle(s);
}

GeomOffsetSurface::~GeomOffsetSurface()
{
}

void GeomOffsetSurface::setHandle(const Handle(Geom_OffsetSurface)& s)
{
    mySurface = Handle(Geom_OffsetSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomOffsetSurface::handle() const
{
    return mySurface;
}

Geometry *GeomOffsetSurface::copy(void) const
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

TYPESYSTEM_SOURCE(Part::GeomPlateSurface,Part::GeomSurface)

GeomPlateSurface::GeomPlateSurface()
{
}

GeomPlateSurface::GeomPlateSurface(const Handle(Geom_Surface)& s, const Plate_Plate& plate)
{
    this->mySurface = new GeomPlate_Surface(s, plate);
}

GeomPlateSurface::GeomPlateSurface(const GeomPlate_BuildPlateSurface& buildPlate)
{
    Handle(GeomPlate_Surface) s = buildPlate.Surface();
    this->mySurface = Handle(GeomPlate_Surface)::DownCast(s->Copy());
}

GeomPlateSurface::GeomPlateSurface(const Handle(GeomPlate_Surface)& s)
{
    setHandle(s);
}

GeomPlateSurface::~GeomPlateSurface()
{
}

void GeomPlateSurface::setHandle(const Handle(GeomPlate_Surface)& s)
{
    mySurface = Handle(GeomPlate_Surface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomPlateSurface::handle() const
{
    return mySurface;
}

Geometry *GeomPlateSurface::copy(void) const
{
    GeomPlateSurface *newSurf = new GeomPlateSurface(mySurface);
    newSurf->Construction = this->Construction;
    return newSurf;
}

// Persistence implementer
unsigned int GeomPlateSurface::getMemSize (void) const
{
    throw Base::NotImplementedError("GeomPlateSurface::getMemSize");
}

void GeomPlateSurface::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomPlateSurface::Save");
}

void GeomPlateSurface::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomPlateSurface::Restore");
}

PyObject *GeomPlateSurface::getPyObject(void)
{
    return new PlateSurfacePy(static_cast<GeomPlateSurface*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomTrimmedSurface,Part::GeomSurface)

GeomTrimmedSurface::GeomTrimmedSurface()
{
}

GeomTrimmedSurface::GeomTrimmedSurface(const Handle(Geom_RectangularTrimmedSurface)& s)
{
   setHandle(s);
}

GeomTrimmedSurface::~GeomTrimmedSurface()
{
}

void GeomTrimmedSurface::setHandle(const Handle(Geom_RectangularTrimmedSurface)& s)
{
    mySurface = Handle(Geom_RectangularTrimmedSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomTrimmedSurface::handle() const
{
    return mySurface;
}

Geometry *GeomTrimmedSurface::copy(void) const
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

TYPESYSTEM_SOURCE(Part::GeomSurfaceOfRevolution,Part::GeomSurface)

GeomSurfaceOfRevolution::GeomSurfaceOfRevolution()
{
}

GeomSurfaceOfRevolution::GeomSurfaceOfRevolution(const Handle(Geom_Curve)& c, const gp_Ax1& a)
{
    this->mySurface = new Geom_SurfaceOfRevolution(c,a);
}

GeomSurfaceOfRevolution::GeomSurfaceOfRevolution(const Handle(Geom_SurfaceOfRevolution)& s)
{
    setHandle(s);
}

GeomSurfaceOfRevolution::~GeomSurfaceOfRevolution()
{
}

void GeomSurfaceOfRevolution::setHandle(const Handle(Geom_SurfaceOfRevolution)& c)
{
    mySurface = Handle(Geom_SurfaceOfRevolution)::DownCast(c->Copy());
}

const Handle(Geom_Geometry)& GeomSurfaceOfRevolution::handle() const
{
    return mySurface;
}

Geometry *GeomSurfaceOfRevolution::copy(void) const
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

TYPESYSTEM_SOURCE(Part::GeomSurfaceOfExtrusion,Part::GeomSurface)

GeomSurfaceOfExtrusion::GeomSurfaceOfExtrusion()
{
}

GeomSurfaceOfExtrusion::GeomSurfaceOfExtrusion(const Handle(Geom_Curve)& c, const gp_Dir& d)
{
    this->mySurface = new Geom_SurfaceOfLinearExtrusion(c,d);
}

GeomSurfaceOfExtrusion::GeomSurfaceOfExtrusion(const Handle(Geom_SurfaceOfLinearExtrusion)& s)
{
    setHandle(s);
}

GeomSurfaceOfExtrusion::~GeomSurfaceOfExtrusion()
{
}

void GeomSurfaceOfExtrusion::setHandle(const Handle(Geom_SurfaceOfLinearExtrusion)& c)
{
    mySurface = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast(c->Copy());
}

const Handle(Geom_Geometry)& GeomSurfaceOfExtrusion::handle() const
{
    return mySurface;
}

Geometry *GeomSurfaceOfExtrusion::copy(void) const
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
    normPnt1.ProjectToLine(refPnt1-l1p1, l1p2-l1p1);
    normPnt2.ProjectToLine(refPnt2-l2p1, l2p2-l2p1);
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
    projPnt1.ProjectToLine(refPnt1-corner, dir1);
    projPnt2.ProjectToLine(refPnt2-corner, dir2);
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
    radDir1.ProjectToLine(center - corner, dir1);
    radDir2.ProjectToLine(center - corner, dir2);

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
    arc->setRange(startAngle, endAngle, /*emulateCCWXY=*/true);

    return arc;
}

GeomSurface* makeFromSurface(const Handle(Geom_Surface)& s)
{
    if (s->IsKind(STANDARD_TYPE(Geom_ToroidalSurface))) {
        Handle(Geom_ToroidalSurface) hSurf = Handle(Geom_ToroidalSurface)::DownCast(s);
        return new GeomToroid(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_BezierSurface))) {
        Handle(Geom_BezierSurface) hSurf = Handle(Geom_BezierSurface)::DownCast(s);
        return new GeomBezierSurface(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_BSplineSurface))) {
        Handle(Geom_BSplineSurface) hSurf = Handle(Geom_BSplineSurface)::DownCast(s);
        return new GeomBSplineSurface(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_CylindricalSurface))) {
        Handle(Geom_CylindricalSurface) hSurf = Handle(Geom_CylindricalSurface)::DownCast(s);
        return new GeomCylinder(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_ConicalSurface))) {
        Handle(Geom_ConicalSurface) hSurf = Handle(Geom_ConicalSurface)::DownCast(s);
        return new GeomCone(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_SphericalSurface))) {
        Handle(Geom_SphericalSurface) hSurf = Handle(Geom_SphericalSurface)::DownCast(s);
        return new GeomSphere(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_Plane))) {
        Handle(Geom_Plane) hSurf = Handle(Geom_Plane)::DownCast(s);
        return new GeomPlane(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_OffsetSurface))) {
        Handle(Geom_OffsetSurface) hSurf = Handle(Geom_OffsetSurface)::DownCast(s);
        return new GeomOffsetSurface(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(GeomPlate_Surface))) {
        Handle(GeomPlate_Surface) hSurf = Handle(GeomPlate_Surface)::DownCast(s);
        return new GeomPlateSurface(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_RectangularTrimmedSurface))) {
        Handle(Geom_RectangularTrimmedSurface) hSurf = Handle(Geom_RectangularTrimmedSurface)::DownCast(s);
        return new GeomTrimmedSurface(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_SurfaceOfRevolution))) {
        Handle(Geom_SurfaceOfRevolution) hSurf = Handle(Geom_SurfaceOfRevolution)::DownCast(s);
        return new GeomSurfaceOfRevolution(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_SurfaceOfLinearExtrusion))) {
        Handle(Geom_SurfaceOfLinearExtrusion) hSurf = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast(s);
        return new GeomSurfaceOfExtrusion(hSurf);
    }

    std::string err = "Unhandled surface type ";
    err += s->DynamicType()->Name();
    throw Base::TypeError(err);
}

}
