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
# include <Approx_Curve3d.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BSplCLib.hxx>
# include <GC_MakeArcOfCircle.hxx>
# include <GC_MakeArcOfEllipse.hxx>
# include <GC_MakeArcOfHyperbola.hxx>
# include <GC_MakeArcOfParabola.hxx>
# include <GC_MakeCircle.hxx>
# include <GC_MakeEllipse.hxx>
# include <GC_MakeHyperbola.hxx>
# include <GC_MakeSegment.hxx>
# include <GCPnts_AbscissaPoint.hxx>
# include <gce_ErrorType.hxx>
# include <gce_MakeParab.hxx>
# include <Geom_BezierCurve.hxx>
# include <Geom_BezierSurface.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_BSplineSurface.hxx>
# include <Geom_CartesianPoint.hxx>
# include <Geom_Circle.hxx>
# include <Geom_ConicalSurface.hxx>
# include <Geom_Curve.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom_Ellipse.hxx>
# include <Geom_Hyperbola.hxx>
# include <Geom_Line.hxx>
# include <Geom_OffsetCurve.hxx>
# include <Geom_OffsetSurface.hxx>
# include <Geom_Parabola.hxx>
# include <Geom_Plane.hxx>
# include <Geom_RectangularTrimmedSurface.hxx>
# include <Geom_SphericalSurface.hxx>
# include <Geom_Surface.hxx>
# include <Geom_SurfaceOfLinearExtrusion.hxx>
# include <Geom_SurfaceOfRevolution.hxx>
# include <Geom_ToroidalSurface.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <GeomAPI_ExtremaCurveCurve.hxx>
# include <GeomAPI_Interpolate.hxx>
# include <GeomAPI_ProjectPointOnCurve.hxx>
# include <GeomConvert.hxx>
# include <GeomConvert_CompCurveToBSplineCurve.hxx>
# include <GeomLProp_CLProps.hxx>
# include <GeomLProp_SLProps.hxx>
# include <GeomPlate_Surface.hxx>
# include <gp_Ax2.hxx>
# include <gp_Circ.hxx>
# include <gp_Cone.hxx>
# include <gp_Cylinder.hxx>
# include <gp_Elips.hxx>
# include <gp_Hypr.hxx>
# include <gp_Lin.hxx>
# include <gp_Parab.hxx>
# include <gp_Pln.hxx>
# include <gp_Pnt.hxx>
# include <gp_Sphere.hxx>
# include <gp_Torus.hxx>
# include <LProp_NotDefined.hxx>
# include <Precision.hxx>
# include <ShapeConstruct_Curve.hxx>
# include <Standard_ConstructionError.hxx>
# include <Standard_Real.hxx>
# include <Standard_Version.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TColgp_Array1OfVec.hxx>
# include <TColgp_Array2OfPnt.hxx>
# include <TColgp_HArray1OfPnt.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <TColStd_HArray1OfBoolean.hxx>

# if OCC_VERSION_HEX < 0x070600
# include <GeomAdaptor_HCurve.hxx>
# endif

# include <cmath>
# include <ctime>
#endif //_PreComp_

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "Geometry.h"
#include "ArcOfCirclePy.h"
#include "ArcOfEllipsePy.h"
#include "ArcOfHyperbolaPy.h"
#include "ArcOfParabolaPy.h"
#include "BezierCurvePy.h"
#include "BezierSurfacePy.h"
#include "BSplineCurveBiArcs.h"
#include "BSplineCurvePy.h"
#include "BSplineSurfacePy.h"
#include "CirclePy.h"
#include "ConePy.h"
#include "CylinderPy.h"
#include "EllipsePy.h"
#include "GeometryMigrationExtension.h"
#include "HyperbolaPy.h"
#include "LinePy.h"
#include "LineSegmentPy.h"
#include "OffsetCurvePy.h"
#include "OffsetSurfacePy.h"
#include "ParabolaPy.h"
#include "PlanePy.h"
#include "PlateSurfacePy.h"
#include "PointPy.h"
#include "RectangularTrimmedSurfacePy.h"
#include "SpherePy.h"
#include "SurfaceOfExtrusionPy.h"
#include "SurfaceOfRevolutionPy.h"
#include "Tools.h"
#include "ToroidPy.h"


#if OCC_VERSION_HEX >= 0x070600
using GeomAdaptor_HCurve = GeomAdaptor_Curve;
#endif

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
{
    createNewTag();
}

Geometry::~Geometry() = default;

// Persistence implementer
unsigned int Geometry::getMemSize () const
{
    return 1;
}

void Geometry::Save(Base::Writer &writer) const
{
    // We always store an extension array even if empty, so that restoring is consistent.

    // Get the number of persistent extensions
    int counter = 0;
    for(const auto& att : extensions) {
        if(att->isDerivedFrom(Part::GeometryPersistenceExtension::getClassTypeId()))
            counter++;
    }

    writer.Stream() << writer.ind() << "<GeoExtensions count=\"" << counter << "\">" << std::endl;

    writer.incInd();

    for(const auto& att : extensions) {
        if(att->isDerivedFrom(Part::GeometryPersistenceExtension::getClassTypeId()))
            std::static_pointer_cast<Part::GeometryPersistenceExtension>(att)->Save(writer);
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</GeoExtensions>" << std::endl;
}

void Geometry::Restore(Base::XMLReader &reader)
{
    // In legacy file format, there are no extensions and there is a construction XML tag
    // In the new format, this is migrated into extensions, and we get an array with extensions
    reader.readElement();

    if(strcmp(reader.localName(),"GeoExtensions") == 0) { // new format

        long count = reader.getAttributeAsInteger("count");

        for (long i = 0; i < count; i++) {
            reader.readElement("GeoExtension");
            const char* TypeName = reader.getAttribute("type");
            Base::Type type = Base::Type::fromName(TypeName);
            GeometryPersistenceExtension *newE = static_cast<GeometryPersistenceExtension *>(type.createInstance());
            if (newE) {
                newE->Restore(reader);

                extensions.push_back(std::shared_ptr<GeometryExtension>(newE));
            }
            else {
                Base::Console().Warning("Cannot restore geometry extension of type: %s\n", TypeName);
            }
        }

        reader.readEndElement("GeoExtensions");
    }
    else if(strcmp(reader.localName(),"Construction") == 0) { // legacy

        bool construction = (int)reader.getAttributeAsInteger("value")==0?false:true;

        // prepare migration
        if(!this->hasExtension(GeometryMigrationExtension::getClassTypeId()))
            this->setExtension(std::make_unique<GeometryMigrationExtension>());

        auto ext = std::static_pointer_cast<GeometryMigrationExtension>(this->getExtension(GeometryMigrationExtension::getClassTypeId()).lock());

        ext->setMigrationType(GeometryMigrationExtension::Construction);
        ext->setConstruction(construction);

    }

}

boost::uuids::uuid Geometry::getTag() const
{
    return tag;
}

std::vector<std::weak_ptr<const GeometryExtension>> Geometry::getExtensions() const
{
    std::vector<std::weak_ptr<const GeometryExtension>> wp;

    for(auto & ext:extensions)
        wp.push_back(ext);

    return wp;
}

bool Geometry::hasExtension(const Base::Type & type) const
{
    for(const auto& ext : extensions) {
        if(ext->getTypeId() == type)
            return true;
    }

    return false;
}

bool Geometry::hasExtension(const std::string & name) const
{
    for(const auto& ext : extensions) {
        if(ext->getName() == name)
            return true;
    }

    return false;
}

std::weak_ptr<GeometryExtension> Geometry::getExtension(const Base::Type & type)
{
    for(const auto& ext : extensions) {
        if(ext->getTypeId() == type)
            return ext;
    }

    throw Base::ValueError("No geometry extension of the requested type.");
}

std::weak_ptr<GeometryExtension> Geometry::getExtension(const std::string & name)
{
    for(const auto& ext : extensions) {
        if(ext->getName() == name)
            return ext;
    }

    throw Base::ValueError("No geometry extension with the requested name.");
}

std::weak_ptr<const GeometryExtension> Geometry::getExtension(const Base::Type & type) const
{
    return const_cast<Geometry*>(this)->getExtension(type).lock();
}

std::weak_ptr<const GeometryExtension> Geometry::getExtension(const std::string & name) const
{
    return const_cast<Geometry*>(this)->getExtension(name).lock();
}


void Geometry::setExtension(std::unique_ptr<GeometryExtension> && geoext )
{
    bool hasext=false;

    for( auto & ext : extensions) {
        // if same type and name, this modifies the existing extension.
        if( ext->getTypeId() == geoext->getTypeId() &&
            ext->getName() == geoext->getName()){
            ext = std::move( geoext );
            ext->notifyAttachment(this);
            hasext = true;
            break;
        }
    }

    if(!hasext) { // new type-name unique id, so add.
        extensions.push_back(std::move( geoext ));
        extensions.back()->notifyAttachment(this);
    }
}

void Geometry::deleteExtension(const Base::Type & type)
{
    extensions.erase(
        std::remove_if( extensions.begin(),
                        extensions.end(),
                        [&type](const std::shared_ptr<GeometryExtension>& ext){
                            return ext->getTypeId() == type;
                        }),
        extensions.end());
}

void Geometry::deleteExtension(const std::string & name)
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
        ran.seed(static_cast<unsigned int>(std::time(nullptr)));
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

void Geometry::copyNonTag(const Part::Geometry * src)
{
    for(auto & ext: src->extensions) {
        this->extensions.push_back(ext->copy());
        extensions.back()->notifyAttachment(this);
    }
}

Geometry *Geometry::clone() const
{
    Geometry* cpy = this->copy();
    cpy->tag = this->tag;

    // class copy is responsible for copying extensions

    return cpy;
}

void Geometry::mirror(const Base::Vector3d& point)
{
    gp_Pnt pnt(point.x, point.y, point.z);
    handle()->Mirror(pnt);
}

void Geometry::mirror(const Base::Vector3d& point, const Base::Vector3d& dir)
{
    gp_Ax1 ax1(gp_Pnt(point.x,point.y,point.z), gp_Dir(dir.x,dir.y,dir.z));
    handle()->Mirror(ax1);
}

void Geometry::rotate(const Base::Placement& plm)
{
    Base::Rotation rot(plm.getRotation());
    Base::Vector3d pnt, dir;

    double angle;

    rot.getValue(dir, angle);
    pnt = plm.getPosition();

    gp_Ax1 ax1(gp_Pnt(pnt.x,pnt.y,pnt.z), gp_Dir(dir.x,dir.y,dir.z));
    handle()->Rotate(ax1, angle);
}

void Geometry::scale(const Base::Vector3d& vec, double scale)
{
    gp_Pnt pnt(vec.x, vec.y, vec.z);
    handle()->Scale(pnt, scale);
}

void Geometry::transform(const Base::Matrix4D& mat)
{
    gp_Trsf trf;
    trf.SetValues(mat[0][0],mat[0][1],mat[0][2],mat[0][3],
                mat[1][0],mat[1][1],mat[1][2],mat[1][3],
                mat[2][0],mat[2][1],mat[2][2],mat[2][3]);
    handle()->Transform(trf);
}

void Geometry::translate(const Base::Vector3d& vec)
{
    gp_Vec trl(vec.x, vec.y, vec.z);
    handle()->Translate(trl);
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

GeomPoint::~GeomPoint() = default;

const Handle(Geom_Geometry)& GeomPoint::handle() const
{
    return myPoint;
}


void GeomPoint::setHandle(const Handle(Geom_CartesianPoint)& p)
{
    myPoint = Handle(Geom_CartesianPoint)::DownCast(p->Copy());
}

Geometry *GeomPoint::copy() const
{
    GeomPoint *newPoint = new GeomPoint(myPoint);
    newPoint->copyNonTag(this);
    return newPoint;
}

TopoDS_Shape GeomPoint::toShape() const
{
    return BRepBuilderAPI_MakeVertex(myPoint->Pnt());
}

Base::Vector3d GeomPoint::getPoint()const
{
    return Base::Vector3d(myPoint->X(),myPoint->Y(),myPoint->Z());
}

void GeomPoint::setPoint(const Base::Vector3d& p)
{
    this->myPoint->SetCoord(p.x,p.y,p.z);
}

// Persistence implementer
unsigned int GeomPoint::getMemSize () const
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
             "\"/>" << std::endl;
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

PyObject *GeomPoint::getPyObject()
{
    return new PointPy(new GeomPoint(getPoint()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Part::GeomCurve,Part::Geometry)

GeomCurve::GeomCurve() = default;

GeomCurve::~GeomCurve() = default;

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

Base::Vector3d GeomCurve::value(double u) const
{
    Handle(Geom_Curve) c = Handle(Geom_Curve)::DownCast(handle());

    const gp_Pnt &point = c->Value(u);

    return Base::Vector3d(point.X(),point.Y(),point.Z());
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

bool GeomCurve::normalAt(const Base::Vector3d & curvepoint, Base::Vector3d & dir) const
{
    double u;
    closestParameter(curvepoint, u);

    return normalAt(u, dir);
}

bool GeomCurve::intersect(  const GeomCurve *c,
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
    // https://forum.freecad.org/viewtopic.php?f=10&t=31700
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
        if(!points.empty())
            return true;
        else
            THROWM(Base::CADKernelError,e.GetMessageString())
    }


    return !points.empty()?true:false;
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

void GeomCurve::reverse()
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

GeomBoundedCurve::GeomBoundedCurve() = default;

GeomBoundedCurve::~GeomBoundedCurve() = default;

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

GeomBezierCurve::~GeomBezierCurve() = default;

void GeomBezierCurve::setHandle(const Handle(Geom_BezierCurve)& c)
{
    myCurve = Handle(Geom_BezierCurve)::DownCast(c->Copy());
}

const Handle(Geom_Geometry)& GeomBezierCurve::handle() const
{
    return myCurve;
}

Geometry *GeomBezierCurve::copy() const
{
    GeomBezierCurve *newCurve = new GeomBezierCurve(myCurve);
    newCurve->copyNonTag(this);
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
        poles.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
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
unsigned int GeomBezierCurve::getMemSize () const
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
             "\">" << std::endl;

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
        "\"/>" << std::endl;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</BezierCurve>" << std::endl ;
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

PyObject *GeomBezierCurve::getPyObject()
{
    return new BezierCurvePy(static_cast<GeomBezierCurve*>(this->clone()));
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


GeomBSplineCurve::~GeomBSplineCurve() = default;

void GeomBSplineCurve::setHandle(const Handle(Geom_BSplineCurve)& c)
{
    myCurve = Handle(Geom_BSplineCurve)::DownCast(c->Copy());
}

const Handle(Geom_Geometry)& GeomBSplineCurve::handle() const
{
    return myCurve;
}

Geometry *GeomBSplineCurve::copy() const
{
    try {
        GeomBSplineCurve *newCurve = new GeomBSplineCurve(myCurve);
        newCurve->copyNonTag(this);
        return newCurve;
    }
    catch (Standard_Failure& e) {
        THROWM(Base::CADKernelError, e.GetMessageString())
    }
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

std::list<Geometry*> GeomBSplineCurve::toBiArcs(double tolerance) const
{
    BSplineCurveBiArcs arcs(this->myCurve);
    return arcs.toBiArcs(tolerance);
}

void GeomBSplineCurve::workAroundOCCTBug(const std::vector<double>& weights)
{
    // If during assignment of weights (during the for loop below) all weights
    // become (temporarily) equal even though weights does not have equal values
    // OCCT will convert all the weights (the already assigned and those not yet assigned)
    // to 1.0 (nonrational b-splines have 1.0 weights). This may lead to the assignment of wrong
    // of weight values.
    //
    // Little hack is to set the last weight to a value different from last but one current and to-be-assigned

    if (weights.size() < 2) // at least two poles/weights
        return;

    auto lastindex = myCurve->NbPoles(); // OCCT is base-1
    auto lastbutonevalue = myCurve->Weight(lastindex-1);
    double fakelastvalue = lastbutonevalue + weights[weights.size()-2];
    myCurve->SetWeight(weights.size(),fakelastvalue);
}

void GeomBSplineCurve::setPoles(const std::vector<Base::Vector3d>& poles, const std::vector<double>& weights)
{
    if (poles.size() != weights.size())
        throw Base::ValueError("poles and weights mismatch");

    workAroundOCCTBug(weights);

    Standard_Integer index=1;

    for (std::size_t i = 0; i < poles.size(); i++, index++) {
        setPole(index, poles[i], weights[i]);
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
        poles.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
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
    workAroundOCCTBug(weights);

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

void GeomBSplineCurve::setPeriodic() const
{
    myCurve->SetPeriodic();
}

bool GeomBSplineCurve::isRational() const
{
    return myCurve->IsRational()==Standard_True;
}

bool GeomBSplineCurve::join(const Handle(Geom_BoundedCurve)& other)
{
    GeomConvert_CompCurveToBSplineCurve ccbc(this->myCurve);
    if (!ccbc.Add(other, Precision::Approximation()))
        return false;
    this->myCurve = ccbc.BSplineCurve();
    return true;
}

void GeomBSplineCurve::interpolate(const std::vector<gp_Pnt>& p, Standard_Boolean periodic)
{
    if (p.size() < 2)
        Standard_ConstructionError::Raise();

    double tol3d = Precision::Approximation();
    Handle(TColgp_HArray1OfPnt) pts = new TColgp_HArray1OfPnt(1, p.size());
    for (std::size_t i=0; i<p.size(); i++) {
        pts->SetValue(i+1, p[i]);
    }

    GeomAPI_Interpolate interpolate(pts, periodic, tol3d);
    interpolate.Perform();
    this->myCurve = interpolate.Curve();
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

void GeomBSplineCurve::increaseDegree(int degree)
{
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(this->handle());
        curve->IncreaseDegree(degree);
    }
    catch (Standard_Failure& e) {
        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

/*!
 * \brief GeomBSplineCurve::approximate
 * \param tol3d
 * \param maxSegments
 * \param maxDegree
 * \param continuity
 * \return true if the approximation succeeded, false otherwise
 */
bool GeomBSplineCurve::approximate(double tol3d, int maxSegments, int maxDegree, int continuity)
{
    try {
        GeomAbs_Shape cont = GeomAbs_C0;
        if (continuity >= 0 && continuity <= 6)
            cont = static_cast<GeomAbs_Shape>(continuity);

        GeomAdaptor_Curve adapt(myCurve);
        Handle(GeomAdaptor_HCurve) hCurve = new GeomAdaptor_HCurve(adapt);
        Approx_Curve3d approx(hCurve, tol3d, cont, maxSegments, maxDegree);
        if (approx.IsDone() && approx.HasResult()) {
            this->setHandle(approx.Curve());
            return true;
        }
    }
    catch (Standard_Failure& e) {
        THROWM(Base::CADKernelError,e.GetMessageString())
    }

    return false;
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

void GeomBSplineCurve::insertKnot(double param, int multiplicity)
{
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(this->handle());
        curve->InsertKnot(param, multiplicity);
        return;
    }
    catch (Standard_Failure& e) {
        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

bool GeomBSplineCurve::removeKnot(int index, int multiplicity, double tolerance)
{
    try {
        Handle(Geom_BSplineCurve) curve =Handle(Geom_BSplineCurve)::DownCast(myCurve->Copy());
        if (curve->RemoveKnot(index, multiplicity, tolerance)) {

            // It can happen that OCCT computes a negative weight but still claims the removal was successful
            TColStd_Array1OfReal weights(1, curve->NbPoles());
            curve->Weights(weights);
            for (Standard_Integer i = weights.Lower(); i <= weights.Upper(); i++) {
                double v = weights(i);
                if (v <= gp::Resolution())
                    return false;
            }

            myCurve = curve;
            return true;
        }

        return false;
    }
    catch (Standard_Failure& e) {
        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

void GeomBSplineCurve::Trim(double u, double v)
{
    auto splitUnwrappedBSpline = [this](double u, double v) {
        // it makes a copy internally (checked in the source code of OCCT)
        auto handle = GeomConvert::SplitBSplineCurve (  myCurve,
                                                            u,
                                                            v,
                                                            Precision::Confusion()
                                                        );
        setHandle(handle);
    };

    try {
        if (isPeriodic() && (v < u))
            v = v + myCurve->LastParameter() - myCurve->FirstParameter(); // v needs one extra lap
        splitUnwrappedBSpline(u, v);
    }
    catch (Standard_Failure& e) {
        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

void GeomBSplineCurve::scaleKnotsToBounds(double u0, double u1)
{
    try {
        Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(myCurve->Copy());
        Standard_RangeError_Raise_if (u1 <= u0, " ");
        TColStd_Array1OfReal k(1,curve->NbKnots());
        curve->Knots(k);
        if ((abs(u0-k.First()) > Precision::Confusion()) || (abs(u1-k.Last()) > Precision::Confusion())) {
            BSplCLib::Reparametrize(u0, u1, k);
            curve->SetKnots(k);
        }
        myCurve = curve;
        return;
    }
    catch (Standard_Failure& e) {
        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

// Persistence implementer
unsigned int GeomBSplineCurve::getMemSize () const
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
             "\">" << std::endl;

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
        "\"/>" << std::endl;
    }

    std::vector<double>::const_iterator itk;
    std::vector<int>::const_iterator itm;

    for (itk = knots.begin(), itm = mults.begin(); itk != knots.end() && itm != mults.end(); ++itk, ++itm) {
        writer.Stream()
            << writer.ind()
            << "<Knot "
            << "Value=\"" << (*itk)
            << "\" Mult=\"" << (*itm) <<
        "\"/>" << std::endl;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</BSplineCurve>" << std::endl ;
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
    // Base::asBoolean(periodic),
    // Base::asBoolean(CheckRational));

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


PyObject *GeomBSplineCurve::getPyObject()
{
    return new BSplineCurvePy(static_cast<GeomBSplineCurve*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Part::GeomConic, Part::GeomCurve)

GeomConic::GeomConic() = default;

GeomConic::~GeomConic() = default;

Base::Vector3d GeomConic::getLocation() const
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

Base::Vector3d GeomConic::getCenter() const
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

Base::Vector3d GeomConic::getAxisDirection() const
{
    Handle(Geom_Conic) conic =  Handle(Geom_Conic)::DownCast(handle());
    gp_Ax1 axis = conic->Axis();
    const gp_Dir& dir = axis.Direction();
    return Base::Vector3d( dir.X(),dir.Y(),dir.Z());
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
double GeomConic::getAngleXU() const
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

GeomBSplineCurve* GeomConic::toNurbs(double first, double last) const
{
    Handle(Geom_Conic) conic =  Handle(Geom_Conic)::DownCast(handle());
    Handle(Geom_Curve) curve = new Geom_TrimmedCurve(conic, first, last);

    // pass the trimmed conic
    Handle(Geom_BSplineCurve) bspline = GeomConvert::CurveToBSplineCurve(curve);
    Standard_Real fnew = bspline->FirstParameter(), lnew = bspline->LastParameter(), UTol;
    if (!bspline->IsPeriodic()) {
        bspline->Resolution(Precision::Confusion(), UTol);
        if (Abs(first - fnew) > UTol || Abs(last - lnew) > UTol) {
            TColStd_Array1OfReal knots(1,bspline->NbKnots());
            bspline->Knots(knots);
            BSplCLib::Reparametrize(first, last, knots);
            bspline->SetKnots(knots);
        }
    }

    return new GeomBSplineCurve(bspline);
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomTrimmedCurve,Part::GeomBoundedCurve)

GeomTrimmedCurve::GeomTrimmedCurve() = default;

GeomTrimmedCurve::GeomTrimmedCurve(const Handle(Geom_TrimmedCurve)& c)
{
    setHandle(c);
}

GeomTrimmedCurve::~GeomTrimmedCurve() = default;

void GeomTrimmedCurve::setHandle(const Handle(Geom_TrimmedCurve)& c)
{
    this->myCurve = Handle(Geom_TrimmedCurve)::DownCast(c->Copy());
}

const Handle(Geom_Geometry)& GeomTrimmedCurve::handle() const
{
    return myCurve;
}

Geometry *GeomTrimmedCurve::copy() const
{
    GeomTrimmedCurve *newCurve =  new GeomTrimmedCurve(myCurve);
    newCurve->copyNonTag(this);
    return newCurve;
}

// Persistence implementer
unsigned int GeomTrimmedCurve::getMemSize () const
{
    return sizeof(Geom_TrimmedCurve);
}

void GeomTrimmedCurve::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomTrimmedCurve::Save");
}

void GeomTrimmedCurve::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomTrimmedCurve::Restore");
}

PyObject *GeomTrimmedCurve::getPyObject()
{
    return new TrimmedCurvePy(static_cast<GeomTrimmedCurve*>(this->clone()));
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

GeomArcOfConic::GeomArcOfConic() = default;

GeomArcOfConic::~GeomArcOfConic() = default;

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

Base::Vector3d GeomArcOfConic::getCenter() const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve->BasisCurve());
    gp_Ax1 axis = conic->Axis();
    const gp_Pnt& loc = axis.Location();
    return Base::Vector3d(loc.X(),loc.Y(),loc.Z());
}

Base::Vector3d GeomArcOfConic::getLocation() const
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

Base::Vector3d GeomArcOfConic::getAxisDirection() const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve->BasisCurve());
    gp_Ax1 axis = conic->Axis();
    const gp_Dir& dir = axis.Direction();
    return Base::Vector3d( dir.X(),dir.Y(),dir.Z());
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
double GeomArcOfConic::getAngleXU() const
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

    if (newdir.Sqr() < Precision::SquareConfusion())
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

GeomCircle::~GeomCircle() = default;

const Handle(Geom_Geometry)& GeomCircle::handle() const
{
    return myCurve;
}


void GeomCircle::setHandle(const Handle(Geom_Circle)& c)
{
    myCurve = Handle(Geom_Circle)::DownCast(c->Copy());
}

Geometry *GeomCircle::copy() const
{
    GeomCircle *newCirc = new GeomCircle(myCurve);
    newCirc->copyNonTag(this);
    return newCirc;
}

GeomBSplineCurve* GeomCircle::toNurbs(double first, double last) const
{
    // for an arc of circle use the generic method
    if (first != 0 || last != 2*M_PI) {
        return GeomConic::toNurbs(first, last);
    }

    Handle(Geom_Circle) conic =  Handle(Geom_Circle)::DownCast(handle());
    double radius = conic->Radius();

    TColgp_Array1OfPnt poles(1, 7);
    poles(1) = gp_Pnt(radius, 0, 0);
    poles(2) = gp_Pnt(radius, 2*radius, 0);
    poles(3) = gp_Pnt(-radius, 2*radius, 0);
    poles(4) = gp_Pnt(-radius, 0, 0);
    poles(5) = gp_Pnt(-radius, -2*radius, 0);
    poles(6) = gp_Pnt(radius, -2*radius, 0);
    poles(7) = gp_Pnt(radius, 0, 0);

    gp_Trsf trsf;
    trsf.SetTransformation(conic->Position(), gp_Ax3());
    TColStd_Array1OfReal weights(1,7);
    for (int i=1; i<=7; i++) {
        poles(i).Transform(trsf);
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
    return new GeomBSplineCurve(spline);
}

double GeomCircle::getRadius() const
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
unsigned int GeomCircle::getMemSize () const
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
             "\"/>" << std::endl;
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

PyObject *GeomCircle::getPyObject()
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

GeomArcOfCircle::~GeomArcOfCircle() = default;

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

Geometry *GeomArcOfCircle::copy() const
{
    GeomArcOfCircle* copy = new GeomArcOfCircle();
    copy->setHandle(this->myCurve);
    copy->copyNonTag(this);
    return copy;
}

GeomBSplineCurve* GeomArcOfCircle::toNurbs(double first, double last) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(curve->BasisCurve());
    return GeomCircle(circle).toNurbs(first, last);
}

double GeomArcOfCircle::getRadius() const
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
 * \brief GeomArcOfCircle::getAngle
 * \param emulateCCWXY: if true, the arc will pretend to be a CCW arc in XY plane.
 * For this to work, the arc must lie in XY plane (i.e. Axis is either +Z or -Z).
 */
double GeomArcOfCircle::getAngle(bool emulateCCWXY) const
{
    double startangle, endangle;
    getRange(startangle, endangle, emulateCCWXY);
    double angle = endangle - startangle;
    return angle;
}

/*!
 * \brief GeomArcOfCircle::getRange
 * \param u [out] start angle of the arc, in radians.
 * \param v [out] end angle of the arc, in radians.
 * \param emulateCCWXY: if true, the arc will pretend to be a CCW arc in XY plane.
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
unsigned int GeomArcOfCircle::getMemSize () const
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
             "\"/>" << std::endl;
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
        GC_MakeArcOfCircle ma(mc.Value()->Circ(), StartAngle, EndAngle, Standard_True);
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

PyObject *GeomArcOfCircle::getPyObject()
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

GeomEllipse::~GeomEllipse() = default;

const Handle(Geom_Geometry)& GeomEllipse::handle() const
{
    return myCurve;
}

void GeomEllipse::setHandle(const Handle(Geom_Ellipse) &e)
{
    this->myCurve = Handle(Geom_Ellipse)::DownCast(e->Copy());
}

Geometry *GeomEllipse::copy() const
{
    GeomEllipse *newEllipse = new GeomEllipse(myCurve);
    newEllipse->copyNonTag(this);
    return newEllipse;
}

GeomBSplineCurve* GeomEllipse::toNurbs(double first, double last) const
{
    // for an arc of ellipse use the generic method
    if (first != 0 || last != 2*M_PI) {
        return GeomConic::toNurbs(first, last);
    }

    Handle(Geom_Ellipse) conic =  Handle(Geom_Ellipse)::DownCast(handle());
    Standard_Real majorRadius = conic->MajorRadius();
    Standard_Real minorRadius = conic->MinorRadius();

    TColgp_Array1OfPnt poles(1, 7);
    poles(1) = gp_Pnt(majorRadius, 0, 0);
    poles(2) = gp_Pnt(majorRadius, 2*minorRadius, 0);
    poles(3) = gp_Pnt(-majorRadius, 2*minorRadius, 0);
    poles(4) = gp_Pnt(-majorRadius, 0, 0);
    poles(5) = gp_Pnt(-majorRadius, -2*minorRadius, 0);
    poles(6) = gp_Pnt(majorRadius, -2*minorRadius, 0);
    poles(7) = gp_Pnt(majorRadius, 0, 0);

    gp_Trsf trsf;
    trsf.SetTransformation(conic->Position(), gp_Ax3());
    TColStd_Array1OfReal weights(1,7);
    for (int i=1; i<=7; i++) {
        poles(i).Transform(trsf);
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

double GeomEllipse::getMajorRadius() const
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

double GeomEllipse::getMinorRadius() const
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
 * \brief GeomEllipse::getMinorAxisDir
 * \return the direction vector (unit-length) of minor axis of the ellipse.
 */
Base::Vector3d GeomEllipse::getMinorAxisDir() const
{
    gp_Dir ydir = myCurve->YAxis().Direction();
    return Base::Vector3d(ydir.X(), ydir.Y(), ydir.Z());
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
    if (newdir.Sqr() < Precision::SquareConfusion())
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
unsigned int GeomEllipse::getMemSize () const
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
            << "/>" << std::endl;
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

PyObject *GeomEllipse::getPyObject()
{
    return new EllipsePy(static_cast<GeomEllipse*>(this->clone()));
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

GeomArcOfEllipse::~GeomArcOfEllipse() = default;

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

Geometry *GeomArcOfEllipse::copy() const
{
    GeomArcOfEllipse* copy = new GeomArcOfEllipse();
    copy->setHandle(this->myCurve);
    copy->copyNonTag(this);
    return copy;
}

GeomBSplineCurve* GeomArcOfEllipse::toNurbs(double first, double last) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(curve->BasisCurve());
    return GeomEllipse(ellipse).toNurbs(first, last);
}

double GeomArcOfEllipse::getMajorRadius() const
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

double GeomArcOfEllipse::getMinorRadius() const
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
    if (newdir.Sqr() < Precision::SquareConfusion())
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
unsigned int GeomArcOfEllipse::getMemSize () const
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
            << "/>" << std::endl;
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

        GC_MakeArcOfEllipse ma(mc.Value()->Elips(), StartAngle, EndAngle, Standard_True);
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

PyObject *GeomArcOfEllipse::getPyObject()
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

GeomHyperbola::~GeomHyperbola() = default;

const Handle(Geom_Geometry)& GeomHyperbola::handle() const
{
    return myCurve;
}


void GeomHyperbola::setHandle(const Handle(Geom_Hyperbola)& c)
{
    myCurve = Handle(Geom_Hyperbola)::DownCast(c->Copy());
}

Geometry *GeomHyperbola::copy() const
{
    GeomHyperbola *newHyp = new GeomHyperbola(myCurve);
    newHyp->copyNonTag(this);
    return newHyp;
}

GeomBSplineCurve* GeomHyperbola::toNurbs(double first, double last) const
{
    return GeomConic::toNurbs(first, last);
}

double GeomHyperbola::getMajorRadius() const
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

double GeomHyperbola::getMinorRadius() const
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
unsigned int GeomHyperbola::getMemSize () const
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
            << "/>" << std::endl;
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

PyObject *GeomHyperbola::getPyObject()
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

GeomArcOfHyperbola::~GeomArcOfHyperbola() = default;

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

Geometry *GeomArcOfHyperbola::copy() const
{
    GeomArcOfHyperbola* copy = new GeomArcOfHyperbola();
    copy->setHandle(this->myCurve);
    copy->copyNonTag(this);
    return copy;
}

GeomBSplineCurve* GeomArcOfHyperbola::toNurbs(double first, double last) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Hyperbola) hyperbola = Handle(Geom_Hyperbola)::DownCast(curve->BasisCurve());
    return GeomHyperbola(hyperbola).toNurbs(first, last);
}

double GeomArcOfHyperbola::getMajorRadius() const
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

double GeomArcOfHyperbola::getMinorRadius() const
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
    if (newdir.Sqr() < Precision::SquareConfusion())
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
unsigned int GeomArcOfHyperbola::getMemSize () const
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
            << "/>" << std::endl;
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

        GC_MakeArcOfHyperbola ma(mc.Value()->Hypr(), StartAngle, EndAngle, Standard_True);
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

PyObject *GeomArcOfHyperbola::getPyObject()
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

GeomParabola::~GeomParabola() = default;

const Handle(Geom_Geometry)& GeomParabola::handle() const
{
    return myCurve;
}

void GeomParabola::setHandle(const Handle(Geom_Parabola)& c)
{
    myCurve = Handle(Geom_Parabola)::DownCast(c->Copy());
}

Geometry *GeomParabola::copy() const
{
    GeomParabola *newPar = new GeomParabola(myCurve);
    newPar->copyNonTag(this);
    return newPar;
}

GeomBSplineCurve* GeomParabola::toNurbs(double first, double last) const
{
    // the default implementation suffices because a non-rational B-spline with
    // one segment is a parabola
    return GeomCurve::toNurbs(first, last);
}

double GeomParabola::getFocal() const
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
unsigned int GeomParabola::getMemSize () const
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
            << "/>" << std::endl;
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

PyObject *GeomParabola::getPyObject()
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

GeomArcOfParabola::~GeomArcOfParabola() = default;

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

Geometry *GeomArcOfParabola::copy() const
{
    GeomArcOfParabola* copy = new GeomArcOfParabola();
    copy->setHandle(this->myCurve);
    copy->copyNonTag(this);
    return copy;
}

GeomBSplineCurve* GeomArcOfParabola::toNurbs(double first, double last) const
{
    Handle(Geom_TrimmedCurve) curve =  Handle(Geom_TrimmedCurve)::DownCast(handle());
    Handle(Geom_Parabola) parabola = Handle(Geom_Parabola)::DownCast(curve->BasisCurve());
    return GeomParabola(parabola).toNurbs(first, last);
}

double GeomArcOfParabola::getFocal() const
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

Base::Vector3d GeomArcOfParabola::getFocus() const
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
unsigned int GeomArcOfParabola::getMemSize () const
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
            << "/>" << std::endl;
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

        GC_MakeArcOfParabola ma(mc.Value(), StartAngle, EndAngle, Standard_True);
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

PyObject *GeomArcOfParabola::getPyObject()
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


GeomLine::~GeomLine() = default;

void GeomLine::setLine(const Base::Vector3d& Pos, const Base::Vector3d& Dir)
{
    this->myCurve->SetLocation(gp_Pnt(Pos.x,Pos.y,Pos.z));
    this->myCurve->SetDirection(gp_Dir(Dir.x,Dir.y,Dir.z));
}

Base::Vector3d GeomLine::getPos() const
{
    gp_Pnt Pos = this->myCurve->Lin().Location();
    return Base::Vector3d(Pos.X(),Pos.Y(),Pos.Z());
}

Base::Vector3d GeomLine::getDir() const
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

Geometry *GeomLine::copy() const
{
    GeomLine *newLine = new GeomLine(myCurve);
    newLine->copyNonTag(this);
    return newLine;
}

// Persistence implementer
unsigned int GeomLine::getMemSize () const
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
             "\"/>" << std::endl;
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

PyObject *GeomLine::getPyObject()
{
    return new LinePy(static_cast<GeomLine*>(this->clone()));
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

GeomLineSegment::~GeomLineSegment() = default;

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

Geometry *GeomLineSegment::copy()const
{
    GeomLineSegment *tempCurve = new GeomLineSegment();
    tempCurve->myCurve = Handle(Geom_TrimmedCurve)::DownCast(myCurve->Copy());
    tempCurve->copyNonTag(this);
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
unsigned int GeomLineSegment::getMemSize () const
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
             "\"/>" << std::endl;
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

PyObject *GeomLineSegment::getPyObject()
{
    return new LineSegmentPy(static_cast<GeomLineSegment*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomOffsetCurve,Part::GeomCurve)

GeomOffsetCurve::GeomOffsetCurve() = default;

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

GeomOffsetCurve::~GeomOffsetCurve() = default;

Geometry *GeomOffsetCurve::copy() const
{
    GeomOffsetCurve *newCurve = new GeomOffsetCurve(myCurve);
    newCurve->copyNonTag(this);
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
unsigned int GeomOffsetCurve::getMemSize () const
{
    return sizeof(Geom_OffsetCurve);
}

void GeomOffsetCurve::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomOffsetCurve::Save");
}

void GeomOffsetCurve::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomOffsetCurve::Restore");
}

PyObject *GeomOffsetCurve::getPyObject()
{
    return new OffsetCurvePy(static_cast<GeomOffsetCurve*>(this->clone()));
}

// -------------------------------------------------


TYPESYSTEM_SOURCE_ABSTRACT(Part::GeomSurface,Part::Geometry)

GeomSurface::GeomSurface() = default;

GeomSurface::~GeomSurface() = default;

TopoDS_Shape GeomSurface::toShape() const
{
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(handle());
    Standard_Real u1,u2,v1,v2;
    s->Bounds(u1,u2,v1,v2);
    BRepBuilderAPI_MakeFace mkBuilder(s, u1, u2, v1, v2, Precision::Confusion() );
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
    Standard_Boolean done;

    Tools::getNormal(s, u, v, Precision::Confusion(), dir, done);

    if (done)
        return true;

    return false;
}

gp_Vec GeomSurface::getDN(double u, double v, int Nu, int Nv) const
{
    Handle(Geom_Surface) s = Handle(Geom_Surface)::DownCast(handle());
    return s->DN(u, v, Nu, Nv);
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

GeomBezierSurface::~GeomBezierSurface() = default;

const Handle(Geom_Geometry)& GeomBezierSurface::handle() const
{
    return mySurface;
}

void GeomBezierSurface::setHandle(const Handle(Geom_BezierSurface)& b)
{
    this->mySurface = Handle(Geom_BezierSurface)::DownCast(b->Copy());
}

Geometry *GeomBezierSurface::copy() const
{
    GeomBezierSurface *newSurf =  new GeomBezierSurface(mySurface);
    newSurf->copyNonTag(this);
    return newSurf;
}

// Persistence implementer
unsigned int GeomBezierSurface::getMemSize () const
{
    unsigned int size = sizeof(Geom_BezierSurface);
    if (!mySurface.IsNull()) {
        unsigned int poles = mySurface->NbUPoles();
        poles *= mySurface->NbVPoles();
        size += poles * sizeof(gp_Pnt);
        size += poles * sizeof(Standard_Real);
    }
    return size;
}

void GeomBezierSurface::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomBezierSurface::Save");
}

void GeomBezierSurface::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomBezierSurface::Restore");
}

PyObject *GeomBezierSurface::getPyObject()
{
    return new BezierSurfacePy(static_cast<GeomBezierSurface*>(this->clone()));
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

GeomBSplineSurface::~GeomBSplineSurface() = default;

void GeomBSplineSurface::setHandle(const Handle(Geom_BSplineSurface)& s)
{
    mySurface = Handle(Geom_BSplineSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomBSplineSurface::handle() const
{
    return mySurface;
}

Geometry *GeomBSplineSurface::copy() const
{
    GeomBSplineSurface *newSurf =  new GeomBSplineSurface(mySurface);
    newSurf->copyNonTag(this);
    return newSurf;
}

void GeomBSplineSurface::scaleKnotsToBounds(double u0, double u1, double v0, double v1)
{
    try {
        Handle(Geom_BSplineSurface) surf = Handle(Geom_BSplineSurface)::DownCast(mySurface->Copy());
        Standard_RangeError_Raise_if (u1 <= u0 || v1 <= v0, " ");
        Standard_Real bu0,bu1,bv0,bv1;
        surf->Bounds(bu0,bu1,bv0,bv1);
        if ((abs(u0-bu0) > Precision::Confusion()) || (abs(u1-bu1) > Precision::Confusion())) {
            TColStd_Array1OfReal uk(1,surf->NbUKnots());
            surf->UKnots(uk);
            BSplCLib::Reparametrize(u0, u1, uk);
            surf->SetUKnots(uk);
        }
        if ((abs(v0-bv0) > Precision::Confusion()) || (abs(v1-bv1) > Precision::Confusion())) {
            TColStd_Array1OfReal vk(1,surf->NbVKnots());
            surf->VKnots(vk);
            BSplCLib::Reparametrize(v0, v1, vk);
            surf->SetVKnots(vk);
        }
        mySurface = surf;
        return;
    }
    catch (Standard_Failure& e) {
        THROWM(Base::CADKernelError,e.GetMessageString())
    }
}

// Persistence implementer
unsigned int GeomBSplineSurface::getMemSize () const
{
    unsigned int size = sizeof(Geom_BSplineSurface);
    if (!mySurface.IsNull()) {
        size += mySurface->NbUKnots() * sizeof(Standard_Real);
        size += mySurface->NbUKnots() * sizeof(Standard_Integer);
        size += mySurface->NbVKnots() * sizeof(Standard_Real);
        size += mySurface->NbVKnots() * sizeof(Standard_Integer);
        unsigned int poles = mySurface->NbUPoles();
        poles *= mySurface->NbVPoles();
        size += poles * sizeof(gp_Pnt);
        size += poles * sizeof(Standard_Real);
    }
    return size;
}

void GeomBSplineSurface::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomBSplineSurface::Save");
}

void GeomBSplineSurface::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomBSplineSurface::Restore");
}

PyObject *GeomBSplineSurface::getPyObject()
{
    return new BSplineSurfacePy(static_cast<GeomBSplineSurface*>(this->clone()));
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

GeomCylinder::~GeomCylinder() = default;

void GeomCylinder::setHandle(const Handle(Geom_CylindricalSurface)& s)
{
    mySurface = Handle(Geom_CylindricalSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomCylinder::handle() const
{
    return mySurface;
}

Geometry *GeomCylinder::copy() const
{
    GeomCylinder *tempCurve = new GeomCylinder();
    tempCurve->mySurface = Handle(Geom_CylindricalSurface)::DownCast(mySurface->Copy());
    tempCurve->copyNonTag(this);
    return tempCurve;
}

// Persistence implementer
unsigned int GeomCylinder::getMemSize () const
{
    return sizeof(Geom_CylindricalSurface);
}

void GeomCylinder::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomCylinder::Save");
}

void GeomCylinder::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomCylinder::Restore");
}

PyObject *GeomCylinder::getPyObject()
{
    return new CylinderPy(static_cast<GeomCylinder*>(this->clone()));
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

GeomCone::~GeomCone() = default;

void GeomCone::setHandle(const Handle(Geom_ConicalSurface)& s)
{
    mySurface = Handle(Geom_ConicalSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomCone::handle() const
{
    return mySurface;
}

Geometry *GeomCone::copy() const
{
    GeomCone *tempCurve = new GeomCone();
    tempCurve->mySurface = Handle(Geom_ConicalSurface)::DownCast(mySurface->Copy());
    tempCurve->copyNonTag(this);
    return tempCurve;
}

// Persistence implementer
unsigned int GeomCone::getMemSize () const
{
    return sizeof(Geom_ConicalSurface);
}

void GeomCone::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomCone::Save");
}

void GeomCone::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomCone::Restore");
}

PyObject *GeomCone::getPyObject()
{
    return new ConePy(static_cast<GeomCone*>(this->clone()));
}

gp_Vec GeomCone::getDN(double u, double v, int Nu, int Nv) const
{
    // Will be fixed in OCC 7.7
#if OCC_VERSION_HEX >= 0x070700
    return GeomSurface::getDN(u, v, Nu, Nv);
#else
    // Copied from ElSLib::ConeDN() and applied the needed fix
    auto ElSLib__ConeDN = [](const Standard_Real U,
                             const Standard_Real V,
                             const gp_Ax3& Pos,
                             const Standard_Real Radius,
                             const Standard_Real SAngle,
                             const Standard_Integer Nu,
                             const Standard_Integer Nv)
    {
       gp_XYZ Xdir = Pos.XDirection().XYZ();
       gp_XYZ Ydir = Pos.YDirection().XYZ();
       Standard_Real Um = U + Nu * M_PI_2;  // M_PI * 0.5
       Xdir.Multiply(cos(Um));
       Ydir.Multiply(sin(Um));
       Xdir.Add(Ydir);
       if(Nv == 0) {
         Xdir.Multiply(Radius + V * sin(SAngle));
         if(Nu == 0) Xdir.Add(Pos.Location().XYZ());
         return gp_Vec(Xdir);
       }
       else if(Nv == 1) {
         Xdir.Multiply(sin(SAngle));
         if (Nu == 0)
           Xdir.Add(Pos.Direction().XYZ() * cos(SAngle));
         return gp_Vec(Xdir);
       }
       return gp_Vec(0.0,0.0,0.0);
    };

    // Workaround for cones to get the correct derivatives
    // https://forum.freecad.org/viewtopic.php?f=10&t=66677
    Handle(Geom_ConicalSurface) s = Handle(Geom_ConicalSurface)::DownCast(handle());
    Standard_RangeError_Raise_if (Nu + Nv < 1 || Nu < 0 || Nv < 0, " ");
    if (Nv > 1) {
        return {0.0, 0.0, 0.0};
    }
    else {
      return ElSLib__ConeDN(u, v, s->Position(), s->RefRadius(), s->SemiAngle(), Nu, Nv);
    }
#endif
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

GeomToroid::~GeomToroid() = default;

void GeomToroid::setHandle(const Handle(Geom_ToroidalSurface)& s)
{
    mySurface = Handle(Geom_ToroidalSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomToroid::handle() const
{
    return mySurface;
}

Geometry *GeomToroid::copy() const
{
    GeomToroid *tempCurve = new GeomToroid();
    tempCurve->mySurface = Handle(Geom_ToroidalSurface)::DownCast(mySurface->Copy());
    tempCurve->copyNonTag(this);
    return tempCurve;
}

// Persistence implementer
unsigned int GeomToroid::getMemSize () const
{
    return sizeof(Geom_ToroidalSurface);
}

void GeomToroid::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomToroid::Save");
}

void GeomToroid::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomToroid::Restore");
}

PyObject *GeomToroid::getPyObject()
{
    return new ToroidPy(static_cast<GeomToroid*>(this->clone()));
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

GeomSphere::~GeomSphere() = default;

void GeomSphere::setHandle(const Handle(Geom_SphericalSurface)& s)
{
    mySurface = Handle(Geom_SphericalSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomSphere::handle() const
{
    return mySurface;
}

Geometry *GeomSphere::copy() const
{
    GeomSphere *tempCurve = new GeomSphere();
    tempCurve->mySurface = Handle(Geom_SphericalSurface)::DownCast(mySurface->Copy());
    tempCurve->copyNonTag(this);
    return tempCurve;
}

// Persistence implementer
unsigned int GeomSphere::getMemSize () const
{
    return sizeof(Geom_SphericalSurface);
}

void GeomSphere::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomSphere::Save");
}

void GeomSphere::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomSphere::Restore");
}

PyObject *GeomSphere::getPyObject()
{
    return new SpherePy(static_cast<GeomSphere*>(this->clone()));
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

GeomPlane::~GeomPlane() = default;

void GeomPlane::setHandle(const Handle(Geom_Plane)& s)
{
    mySurface = Handle(Geom_Plane)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomPlane::handle() const
{
    return mySurface;
}

Geometry *GeomPlane::copy() const
{
    GeomPlane *tempCurve = new GeomPlane();
    tempCurve->mySurface = Handle(Geom_Plane)::DownCast(mySurface->Copy());
    tempCurve->copyNonTag(this);
    return tempCurve;
}

// Persistence implementer
unsigned int GeomPlane::getMemSize () const
{
    return sizeof(Geom_Plane);
}

void GeomPlane::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomPlane::Save");
}

void GeomPlane::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomPlane::Restore");
}

PyObject *GeomPlane::getPyObject()
{
    return new PlanePy(static_cast<GeomPlane*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomOffsetSurface,Part::GeomSurface)

GeomOffsetSurface::GeomOffsetSurface() = default;

GeomOffsetSurface::GeomOffsetSurface(const Handle(Geom_Surface)& s, double offset)
{
    this->mySurface = new Geom_OffsetSurface(s, offset);
}

GeomOffsetSurface::GeomOffsetSurface(const Handle(Geom_OffsetSurface)& s)
{
    setHandle(s);
}

GeomOffsetSurface::~GeomOffsetSurface() = default;

void GeomOffsetSurface::setHandle(const Handle(Geom_OffsetSurface)& s)
{
    mySurface = Handle(Geom_OffsetSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomOffsetSurface::handle() const
{
    return mySurface;
}

Geometry *GeomOffsetSurface::copy() const
{
    GeomOffsetSurface *newSurf = new GeomOffsetSurface(mySurface);
    newSurf->copyNonTag(this);
    return newSurf;
}

// Persistence implementer
unsigned int GeomOffsetSurface::getMemSize () const
{
    return sizeof(Geom_OffsetSurface);
}

void GeomOffsetSurface::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomOffsetSurface::Save");
}

void GeomOffsetSurface::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomOffsetSurface::Restore");
}

PyObject *GeomOffsetSurface::getPyObject()
{
    return new OffsetSurfacePy(static_cast<GeomOffsetSurface*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomPlateSurface,Part::GeomSurface)

GeomPlateSurface::GeomPlateSurface() = default;

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

GeomPlateSurface::~GeomPlateSurface() = default;

void GeomPlateSurface::setHandle(const Handle(GeomPlate_Surface)& s)
{
    mySurface = Handle(GeomPlate_Surface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomPlateSurface::handle() const
{
    return mySurface;
}

Geometry *GeomPlateSurface::copy() const
{
    GeomPlateSurface *newSurf = new GeomPlateSurface(mySurface);
    newSurf->copyNonTag(this);
    return newSurf;
}

// Persistence implementer
unsigned int GeomPlateSurface::getMemSize () const
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

PyObject *GeomPlateSurface::getPyObject()
{
    return new PlateSurfacePy(static_cast<GeomPlateSurface*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomTrimmedSurface,Part::GeomSurface)

GeomTrimmedSurface::GeomTrimmedSurface() = default;

GeomTrimmedSurface::GeomTrimmedSurface(const Handle(Geom_RectangularTrimmedSurface)& s)
{
   setHandle(s);
}

GeomTrimmedSurface::~GeomTrimmedSurface() = default;

void GeomTrimmedSurface::setHandle(const Handle(Geom_RectangularTrimmedSurface)& s)
{
    mySurface = Handle(Geom_RectangularTrimmedSurface)::DownCast(s->Copy());
}

const Handle(Geom_Geometry)& GeomTrimmedSurface::handle() const
{
    return mySurface;
}

Geometry *GeomTrimmedSurface::copy() const
{
    GeomTrimmedSurface *newSurf = new GeomTrimmedSurface(mySurface);
    newSurf->copyNonTag(this);
    return newSurf;
}

// Persistence implementer
unsigned int GeomTrimmedSurface::getMemSize () const
{
    return sizeof(Geom_RectangularTrimmedSurface);
}

void GeomTrimmedSurface::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomTrimmedSurface::Save");
}

void GeomTrimmedSurface::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomTrimmedSurface::Restore");
}

PyObject *GeomTrimmedSurface::getPyObject()
{
    return new RectangularTrimmedSurfacePy(static_cast<GeomTrimmedSurface*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomSurfaceOfRevolution,Part::GeomSurface)

GeomSurfaceOfRevolution::GeomSurfaceOfRevolution() = default;

GeomSurfaceOfRevolution::GeomSurfaceOfRevolution(const Handle(Geom_Curve)& c, const gp_Ax1& a)
{
    this->mySurface = new Geom_SurfaceOfRevolution(c,a);
}

GeomSurfaceOfRevolution::GeomSurfaceOfRevolution(const Handle(Geom_SurfaceOfRevolution)& s)
{
    setHandle(s);
}

GeomSurfaceOfRevolution::~GeomSurfaceOfRevolution() = default;

void GeomSurfaceOfRevolution::setHandle(const Handle(Geom_SurfaceOfRevolution)& c)
{
    mySurface = Handle(Geom_SurfaceOfRevolution)::DownCast(c->Copy());
}

const Handle(Geom_Geometry)& GeomSurfaceOfRevolution::handle() const
{
    return mySurface;
}

Geometry *GeomSurfaceOfRevolution::copy() const
{
    GeomSurfaceOfRevolution *newSurf = new GeomSurfaceOfRevolution(mySurface);
    newSurf->copyNonTag(this);
    return newSurf;
}

// Persistence implementer
unsigned int GeomSurfaceOfRevolution::getMemSize () const
{
    return sizeof(Geom_SurfaceOfRevolution);
}

void GeomSurfaceOfRevolution::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomSurfaceOfRevolution::Save");
}

void GeomSurfaceOfRevolution::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomSurfaceOfRevolution::Restore");
}

PyObject *GeomSurfaceOfRevolution::getPyObject()
{
    return new SurfaceOfRevolutionPy(static_cast<GeomSurfaceOfRevolution*>(this->clone()));
}

// -------------------------------------------------

TYPESYSTEM_SOURCE(Part::GeomSurfaceOfExtrusion,Part::GeomSurface)

GeomSurfaceOfExtrusion::GeomSurfaceOfExtrusion() = default;

GeomSurfaceOfExtrusion::GeomSurfaceOfExtrusion(const Handle(Geom_Curve)& c, const gp_Dir& d)
{
    this->mySurface = new Geom_SurfaceOfLinearExtrusion(c,d);
}

GeomSurfaceOfExtrusion::GeomSurfaceOfExtrusion(const Handle(Geom_SurfaceOfLinearExtrusion)& s)
{
    setHandle(s);
}

GeomSurfaceOfExtrusion::~GeomSurfaceOfExtrusion() = default;

void GeomSurfaceOfExtrusion::setHandle(const Handle(Geom_SurfaceOfLinearExtrusion)& c)
{
    mySurface = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast(c->Copy());
}

const Handle(Geom_Geometry)& GeomSurfaceOfExtrusion::handle() const
{
    return mySurface;
}

Geometry *GeomSurfaceOfExtrusion::copy() const
{
    GeomSurfaceOfExtrusion *newSurf = new GeomSurfaceOfExtrusion(mySurface);
    newSurf->copyNonTag(this);
    return newSurf;
}

// Persistence implementer
unsigned int GeomSurfaceOfExtrusion::getMemSize () const
{
   return sizeof(Geom_SurfaceOfLinearExtrusion);
}

void GeomSurfaceOfExtrusion::Save(Base::Writer &/*writer*/) const
{
    throw Base::NotImplementedError("GeomSurfaceOfExtrusion::Save");
}

void GeomSurfaceOfExtrusion::Restore(Base::XMLReader &/*reader*/)
{
    throw Base::NotImplementedError("GeomSurfaceOfExtrusion::Restore");
}

PyObject *GeomSurfaceOfExtrusion::getPyObject()
{
    return new SurfaceOfExtrusionPy(static_cast<GeomSurfaceOfExtrusion*>(this->clone()));
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
        return nullptr;

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

std::unique_ptr<GeomSurface> makeFromSurface(const Handle(Geom_Surface)& s)
{
    std::unique_ptr<GeomSurface> geoSurf;
    if (s->IsKind(STANDARD_TYPE(Geom_ToroidalSurface))) {
        Handle(Geom_ToroidalSurface) hSurf = Handle(Geom_ToroidalSurface)::DownCast(s);
        geoSurf = std::make_unique<GeomToroid>(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_BezierSurface))) {
        Handle(Geom_BezierSurface) hSurf = Handle(Geom_BezierSurface)::DownCast(s);
        geoSurf = std::make_unique<GeomBezierSurface>(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_BSplineSurface))) {
        Handle(Geom_BSplineSurface) hSurf = Handle(Geom_BSplineSurface)::DownCast(s);
        geoSurf = std::make_unique<GeomBSplineSurface>(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_CylindricalSurface))) {
        Handle(Geom_CylindricalSurface) hSurf = Handle(Geom_CylindricalSurface)::DownCast(s);
        geoSurf = std::make_unique<GeomCylinder>(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_ConicalSurface))) {
        Handle(Geom_ConicalSurface) hSurf = Handle(Geom_ConicalSurface)::DownCast(s);
        geoSurf = std::make_unique<GeomCone>(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_SphericalSurface))) {
        Handle(Geom_SphericalSurface) hSurf = Handle(Geom_SphericalSurface)::DownCast(s);
        geoSurf = std::make_unique<GeomSphere>(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_Plane))) {
        Handle(Geom_Plane) hSurf = Handle(Geom_Plane)::DownCast(s);
        geoSurf = std::make_unique<GeomPlane>(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_OffsetSurface))) {
        Handle(Geom_OffsetSurface) hSurf = Handle(Geom_OffsetSurface)::DownCast(s);
        geoSurf = std::make_unique<GeomOffsetSurface>(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(GeomPlate_Surface))) {
        Handle(GeomPlate_Surface) hSurf = Handle(GeomPlate_Surface)::DownCast(s);
        geoSurf = std::make_unique<GeomPlateSurface>(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_RectangularTrimmedSurface))) {
        Handle(Geom_RectangularTrimmedSurface) hSurf = Handle(Geom_RectangularTrimmedSurface)::DownCast(s);
        geoSurf = std::make_unique<GeomTrimmedSurface>(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_SurfaceOfRevolution))) {
        Handle(Geom_SurfaceOfRevolution) hSurf = Handle(Geom_SurfaceOfRevolution)::DownCast(s);
        geoSurf = std::make_unique<GeomSurfaceOfRevolution>(hSurf);
    }
    else if (s->IsKind(STANDARD_TYPE(Geom_SurfaceOfLinearExtrusion))) {
        Handle(Geom_SurfaceOfLinearExtrusion) hSurf = Handle(Geom_SurfaceOfLinearExtrusion)::DownCast(s);
        geoSurf = std::make_unique<GeomSurfaceOfExtrusion>(hSurf);
    }
    else {
        std::string err = "Unhandled surface type ";
        err += s->DynamicType()->Name();
        throw Base::TypeError(err);
    }

    return geoSurf;
}

std::unique_ptr<GeomCurve> makeFromCurve(const Handle(Geom_Curve)& c)
{
    std::unique_ptr<GeomCurve> geoCurve;
    if (c->IsKind(STANDARD_TYPE(Geom_Circle))) {
        Handle(Geom_Circle) circ = Handle(Geom_Circle)::DownCast(c);
        geoCurve = std::make_unique<GeomCircle>(circ);
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Ellipse))) {
        Handle(Geom_Ellipse) ell = Handle(Geom_Ellipse)::DownCast(c);
        geoCurve = std::make_unique<GeomEllipse>(ell);
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Hyperbola))) {
        Handle(Geom_Hyperbola) hyp = Handle(Geom_Hyperbola)::DownCast(c);
        geoCurve = std::make_unique<GeomHyperbola>(hyp);
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Line))) {
        Handle(Geom_Line) lin = Handle(Geom_Line)::DownCast(c);
        geoCurve = std::make_unique<GeomLine>(lin);
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_OffsetCurve))) {
        Handle(Geom_OffsetCurve) oc = Handle(Geom_OffsetCurve)::DownCast(c);
        geoCurve = std::make_unique<GeomOffsetCurve>(oc);
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Parabola))) {
        Handle(Geom_Parabola) par = Handle(Geom_Parabola)::DownCast(c);
        geoCurve = std::make_unique<GeomParabola>(par);
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
        return makeFromTrimmedCurve(c, c->FirstParameter(), c->LastParameter());
    }
    /*else if (c->IsKind(STANDARD_TYPE(Geom_BoundedCurve))) {
        Handle(Geom_BoundedCurve) bc = Handle(Geom_BoundedCurve)::DownCast(c);
        return Py::asObject(new GeometryCurvePy(new GeomBoundedCurve(bc)));
    }*/
    else if (c->IsKind(STANDARD_TYPE(Geom_BezierCurve))) {
        Handle(Geom_BezierCurve) bezier = Handle(Geom_BezierCurve)::DownCast(c);
        geoCurve = std::make_unique<GeomBezierCurve>(bezier);
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_BSplineCurve))) {
        Handle(Geom_BSplineCurve) bspline = Handle(Geom_BSplineCurve)::DownCast(c);
        geoCurve = std::make_unique<GeomBSplineCurve>(bspline);
    }
    else {
        std::string err = "Unhandled curve type ";
        err += c->DynamicType()->Name();
        throw Base::TypeError(err);
    }

    return geoCurve;
}

std::unique_ptr<GeomCurve> makeFromTrimmedCurve(const Handle(Geom_Curve)& c, double f, double l)
{
    if (c->IsKind(STANDARD_TYPE(Geom_Circle))) {
        Handle(Geom_Circle) circ = Handle(Geom_Circle)::DownCast(c);
        std::unique_ptr<GeomCurve> arc(new GeomArcOfCircle());
        Handle(Geom_TrimmedCurve) this_arc = Handle(Geom_TrimmedCurve)::DownCast
            (arc->handle());
        Handle(Geom_Circle) this_circ = Handle(Geom_Circle)::DownCast
            (this_arc->BasisCurve());
        this_circ->SetCirc(circ->Circ());
        this_arc->SetTrim(f, l);
        return arc;
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Ellipse))) {
        Handle(Geom_Ellipse) ellp = Handle(Geom_Ellipse)::DownCast(c);
        std::unique_ptr<GeomCurve> arc(new GeomArcOfEllipse());
        Handle(Geom_TrimmedCurve) this_arc = Handle(Geom_TrimmedCurve)::DownCast
            (arc->handle());
        Handle(Geom_Ellipse) this_ellp = Handle(Geom_Ellipse)::DownCast
            (this_arc->BasisCurve());
        this_ellp->SetElips(ellp->Elips());
        this_arc->SetTrim(f, l);
        return arc;
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Hyperbola))) {
        Handle(Geom_Hyperbola) hypr = Handle(Geom_Hyperbola)::DownCast(c);
        std::unique_ptr<GeomCurve> arc(new GeomArcOfHyperbola());
        Handle(Geom_TrimmedCurve) this_arc = Handle(Geom_TrimmedCurve)::DownCast
            (arc->handle());
        Handle(Geom_Hyperbola) this_hypr = Handle(Geom_Hyperbola)::DownCast
            (this_arc->BasisCurve());
        this_hypr->SetHypr(hypr->Hypr());
        this_arc->SetTrim(f, l);
        return arc;
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Line))) {
        Handle(Geom_Line) line = Handle(Geom_Line)::DownCast(c);
        std::unique_ptr<GeomCurve> segm(new GeomLineSegment());
        Handle(Geom_TrimmedCurve) this_segm = Handle(Geom_TrimmedCurve)::DownCast
            (segm->handle());
        Handle(Geom_Line) this_line = Handle(Geom_Line)::DownCast
            (this_segm->BasisCurve());
        this_line->SetLin(line->Lin());
        this_segm->SetTrim(f, l);
        return segm;
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_Parabola))) {
        Handle(Geom_Parabola) para = Handle(Geom_Parabola)::DownCast(c);
        std::unique_ptr<GeomCurve> arc(new GeomArcOfParabola());
        Handle(Geom_TrimmedCurve) this_arc = Handle(Geom_TrimmedCurve)::DownCast
            (arc->handle());
        Handle(Geom_Parabola) this_para = Handle(Geom_Parabola)::DownCast
            (this_arc->BasisCurve());
        this_para->SetParab(para->Parab());
        this_arc->SetTrim(f, l);
        return arc;
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_BezierCurve))) {
        Handle(Geom_BezierCurve) bezier = Handle(Geom_BezierCurve)::DownCast(c->Copy());
        bezier->Segment(f, l);
        return std::unique_ptr<GeomCurve>(new GeomBezierCurve(bezier));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_BSplineCurve))) {
        Handle(Geom_BSplineCurve) bspline = Handle(Geom_BSplineCurve)::DownCast(c->Copy());
        bspline->Segment(f, l);
        return std::unique_ptr<GeomCurve>(new GeomBSplineCurve(bspline));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_OffsetCurve))) {
        Handle(Geom_OffsetCurve) oc = Handle(Geom_OffsetCurve)::DownCast(c);
        double v = oc->Offset();
        gp_Dir dir = oc->Direction();
        std::unique_ptr<GeomCurve> bc(makeFromTrimmedCurve(oc->BasisCurve(), f, l));
        return std::unique_ptr<GeomCurve>(new GeomOffsetCurve(Handle(Geom_Curve)::DownCast(bc->handle()), v, dir));
    }
    else if (c->IsKind(STANDARD_TYPE(Geom_TrimmedCurve))) {
        Handle(Geom_TrimmedCurve) trc = Handle(Geom_TrimmedCurve)::DownCast(c);
        return makeFromTrimmedCurve(trc->BasisCurve(), f, l);
    }
    /*else if (c->IsKind(STANDARD_TYPE(Geom_BoundedCurve))) {
        Handle(Geom_BoundedCurve) bc = Handle(Geom_BoundedCurve)::DownCast(c);
        return Py::asObject(new GeometryCurvePy(new GeomBoundedCurve(bc)));
    }*/
    else {
        std::string err = "Unhandled curve type ";
        err += c->DynamicType()->Name();
        throw Base::TypeError(err);
    }
}

std::unique_ptr<GeomCurve> makeFromCurveAdaptor(const Adaptor3d_Curve& adapt)
{
    std::unique_ptr<GeomCurve> geoCurve;
    switch (adapt.GetType())
    {
    case GeomAbs_Line:
        {
            geoCurve = std::make_unique<GeomLine>();
            Handle(Geom_Line) this_curv = Handle(Geom_Line)::DownCast
                (geoCurve->handle());
            this_curv->SetLin(adapt.Line());
            break;
        }
    case GeomAbs_Circle:
        {
            geoCurve = std::make_unique<GeomCircle>();
            Handle(Geom_Circle) this_curv = Handle(Geom_Circle)::DownCast
                (geoCurve->handle());
            this_curv->SetCirc(adapt.Circle());
            break;
        }
    case GeomAbs_Ellipse:
        {
            geoCurve = std::make_unique<GeomEllipse>();
            Handle(Geom_Ellipse) this_curv = Handle(Geom_Ellipse)::DownCast
                (geoCurve->handle());
            this_curv->SetElips(adapt.Ellipse());
            break;
        }
    case GeomAbs_Hyperbola:
        {
            geoCurve = std::make_unique<GeomHyperbola>();
            Handle(Geom_Hyperbola) this_curv = Handle(Geom_Hyperbola)::DownCast
                (geoCurve->handle());
            this_curv->SetHypr(adapt.Hyperbola());
            break;
        }
    case GeomAbs_Parabola:
        {
            geoCurve = std::make_unique<GeomParabola>();
            Handle(Geom_Parabola) this_curv = Handle(Geom_Parabola)::DownCast
                (geoCurve->handle());
            this_curv->SetParab(adapt.Parabola());
            break;
        }
    case GeomAbs_BezierCurve:
        {
            geoCurve = std::make_unique<GeomBezierCurve>(adapt.Bezier());
            break;
        }
    case GeomAbs_BSplineCurve:
        {
            geoCurve = std::make_unique<GeomBSplineCurve>(adapt.BSpline());
            break;
        }
    case GeomAbs_OffsetCurve:
        {
            geoCurve = std::make_unique<GeomOffsetCurve>(adapt.OffsetCurve());
            break;
        }
    case GeomAbs_OtherCurve:
    default:
        break;
    }

    if (!geoCurve)
        throw Base::TypeError("Unhandled curve type");

    // Check if the curve must be trimmed
    Handle(Geom_Curve) curv3d = Handle(Geom_Curve)::DownCast
        (geoCurve->handle());
    double u = curv3d->FirstParameter();
    double v = curv3d->LastParameter();
    if (u != adapt.FirstParameter() || v != adapt.LastParameter()) {
        geoCurve = makeFromTrimmedCurve(curv3d, adapt.FirstParameter(), adapt.LastParameter());
    }

    return geoCurve;
}

}
