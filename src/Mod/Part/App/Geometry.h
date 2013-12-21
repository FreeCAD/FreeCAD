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


#ifndef PART_GEOMETRY_H
#define PART_GEOMETRY_H

#include <Handle_Geom_CartesianPoint.hxx>
#include <Handle_Geom_BezierCurve.hxx>
#include <Handle_Geom_BSplineCurve.hxx>
#include <Handle_Geom_Circle.hxx>
#include <Handle_Geom_Ellipse.hxx>
#include <Handle_Geom_Hyperbola.hxx>
#include <Handle_Geom_Parabola.hxx>
#include <Handle_Geom_Line.hxx>
#include <Handle_Geom_OffsetCurve.hxx>
#include <Handle_Geom_TrimmedCurve.hxx>
#include <Handle_Geom_Surface.hxx>
#include <Handle_Geom_BezierSurface.hxx>
#include <Handle_Geom_BSplineSurface.hxx>
#include <Handle_Geom_CylindricalSurface.hxx>
#include <Handle_Geom_ConicalSurface.hxx>
#include <Handle_Geom_SphericalSurface.hxx>
#include <Handle_Geom_ToroidalSurface.hxx>
#include <Handle_Geom_Plane.hxx>
#include <Handle_Geom_OffsetSurface.hxx>
#include <Handle_Geom_RectangularTrimmedSurface.hxx>
#include <Handle_Geom_SurfaceOfRevolution.hxx>
#include <Handle_Geom_SurfaceOfLinearExtrusion.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

namespace Part {

class PartExport Geometry: public Base::Persistence
{
    TYPESYSTEM_HEADER();
public:
    virtual ~Geometry();

    virtual TopoDS_Shape toShape() const = 0;
    virtual const Handle_Geom_Geometry& handle() const = 0;
    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    /// returns a cloned object 
    virtual Geometry *clone(void) const = 0;
    /// construction geometry (means no impact on a later built topo)
    bool Construction;

protected:
    Geometry();

private:
    Geometry(const Geometry&);
    Geometry& operator = (const Geometry&);
};

class PartExport GeomPoint : public Geometry
{
    TYPESYSTEM_HEADER();
public:
    GeomPoint();
    GeomPoint(const Handle_Geom_CartesianPoint&);
    GeomPoint(const Base::Vector3d&);
    virtual ~GeomPoint();
    virtual Geometry *clone(void) const;
    virtual TopoDS_Shape toShape() const;

   // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;

    Base::Vector3d getPoint(void)const;
    void setPoint(const Base::Vector3d&);

private:
    Handle_Geom_CartesianPoint myPoint;
};

class PartExport GeomCurve : public Geometry
{
    TYPESYSTEM_HEADER();
public:
    GeomCurve();
    virtual ~GeomCurve();

    TopoDS_Shape toShape() const;
    bool tangent(double u, gp_Dir&) const;
};

class PartExport GeomBezierCurve : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomBezierCurve();
    GeomBezierCurve(const Handle_Geom_BezierCurve&);
    virtual ~GeomBezierCurve();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize (void) const;
    virtual void Save (Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_BezierCurve&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_BezierCurve myCurve;
};

class PartExport GeomBSplineCurve : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomBSplineCurve();
    GeomBSplineCurve(const Handle_Geom_BSplineCurve&);
    virtual ~GeomBSplineCurve();
    virtual Geometry *clone(void) const;

    int countPoles() const;
    void setPole(int index, const Base::Vector3d&, double weight=-1);
    std::vector<Base::Vector3d> getPoles() const;
    bool join(const Handle_Geom_BSplineCurve&);
    void makeC1Continuous(double, double);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_BSplineCurve&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_BSplineCurve myCurve;
};

class PartExport GeomCircle : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomCircle();
    GeomCircle(const Handle_Geom_Circle&);
    virtual ~GeomCircle();
    virtual Geometry *clone(void) const;

    Base::Vector3d getCenter(void) const;
    double getRadius(void) const;
    void setCenter(const Base::Vector3d& Center);
    void setRadius(double Radius);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_Circle myCurve;
};

class PartExport GeomArcOfCircle : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomArcOfCircle();
    GeomArcOfCircle(const Handle_Geom_Circle&);
    virtual ~GeomArcOfCircle();
    virtual Geometry *clone(void) const;

    Base::Vector3d getStartPoint() const;
    Base::Vector3d getEndPoint() const;

    Base::Vector3d getCenter(void) const;
    double getRadius(void) const;
    void setCenter(const Base::Vector3d& Center);
    void setRadius(double Radius);
    void getRange(double& u, double& v) const;
    void setRange(double u, double v);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_TrimmedCurve&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_TrimmedCurve myCurve;
};

class PartExport GeomEllipse : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomEllipse();
    GeomEllipse(const Handle_Geom_Ellipse&);
    virtual ~GeomEllipse();
    virtual Geometry *clone(void) const;

    Base::Vector3d getCenter(void) const;
    void setCenter(const Base::Vector3d& Center);
    double getMajorRadius(void) const;
    void setMajorRadius(double Radius);
    double getMinorRadius(void) const;
    void setMinorRadius(double Radius);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_Ellipse myCurve;
};

class PartExport GeomHyperbola : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomHyperbola();
    GeomHyperbola(const Handle_Geom_Hyperbola&);
    virtual ~GeomHyperbola();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_Hyperbola myCurve;
};

class PartExport GeomParabola : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomParabola();
    GeomParabola(const Handle_Geom_Parabola&);
    virtual ~GeomParabola();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_Parabola myCurve;
};

class PartExport GeomLine : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomLine();
    GeomLine(const Handle_Geom_Line&);
    GeomLine(const Base::Vector3d& Pos, const Base::Vector3d& Dir);
    virtual ~GeomLine();
    virtual Geometry *clone(void) const;

    void setLine(const Base::Vector3d& Pos, const Base::Vector3d& Dir);
    Base::Vector3d getPos(void) const;
    Base::Vector3d getDir(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_Line myCurve;
};

class PartExport GeomLineSegment : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomLineSegment();
    virtual ~GeomLineSegment();
    virtual Geometry *clone(void) const;

    Base::Vector3d getStartPoint() const;
    Base::Vector3d getEndPoint() const;

    void setPoints(const Base::Vector3d& p1, 
                   const Base::Vector3d& p2);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_TrimmedCurve&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_TrimmedCurve myCurve;
};

class PartExport GeomOffsetCurve : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomOffsetCurve();
    GeomOffsetCurve(const Handle_Geom_Curve&, double, const gp_Dir&);
    GeomOffsetCurve(const Handle_Geom_OffsetCurve&);
    virtual ~GeomOffsetCurve();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_OffsetCurve& c);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_OffsetCurve myCurve;
};

class PartExport GeomTrimmedCurve : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomTrimmedCurve();
    GeomTrimmedCurve(const Handle_Geom_TrimmedCurve&);
    virtual ~GeomTrimmedCurve();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_TrimmedCurve&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_TrimmedCurve myCurve;
};

class PartExport GeomSurface : public Geometry
{
    TYPESYSTEM_HEADER();
public:
    GeomSurface();
    virtual ~GeomSurface();

    TopoDS_Shape toShape() const;
    bool tangentU(double u, double v, gp_Dir& dirU) const;
    bool tangentV(double u, double v, gp_Dir& dirV) const;
};

class PartExport GeomBezierSurface : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomBezierSurface();
    GeomBezierSurface(const Handle_Geom_BezierSurface&);
    virtual ~GeomBezierSurface();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);


    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_BezierSurface mySurface;
};

class PartExport GeomBSplineSurface : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomBSplineSurface();
    GeomBSplineSurface(const Handle_Geom_BSplineSurface&);
    virtual ~GeomBSplineSurface();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_BSplineSurface&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_BSplineSurface mySurface;
};

class PartExport GeomCylinder : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomCylinder();
    virtual ~GeomCylinder();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_CylindricalSurface mySurface;
};

class PartExport GeomCone : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomCone();
    virtual ~GeomCone();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_ConicalSurface mySurface;
};

class PartExport GeomSphere : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomSphere();
    virtual ~GeomSphere();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_SphericalSurface mySurface;
};

class PartExport GeomToroid : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomToroid();
    virtual ~GeomToroid();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_ToroidalSurface mySurface;
};

class PartExport GeomPlane : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomPlane();
    virtual ~GeomPlane();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_Plane mySurface;
};

class PartExport GeomOffsetSurface : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomOffsetSurface();
    GeomOffsetSurface(const Handle_Geom_Surface&, double);
    GeomOffsetSurface(const Handle_Geom_OffsetSurface&);
    virtual ~GeomOffsetSurface();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_OffsetSurface& s);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_OffsetSurface mySurface;
};

class PartExport GeomTrimmedSurface : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomTrimmedSurface();
    GeomTrimmedSurface(const Handle_Geom_RectangularTrimmedSurface&);
    virtual ~GeomTrimmedSurface();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_RectangularTrimmedSurface& s);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_RectangularTrimmedSurface mySurface;
};

class PartExport GeomSurfaceOfRevolution : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomSurfaceOfRevolution();
    GeomSurfaceOfRevolution(const Handle_Geom_Curve&, const gp_Ax1&);
    GeomSurfaceOfRevolution(const Handle_Geom_SurfaceOfRevolution&);
    virtual ~GeomSurfaceOfRevolution();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_SurfaceOfRevolution& c);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_SurfaceOfRevolution mySurface;
};

class PartExport GeomSurfaceOfExtrusion : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomSurfaceOfExtrusion();
    GeomSurfaceOfExtrusion(const Handle_Geom_Curve&, const gp_Dir&);
    GeomSurfaceOfExtrusion(const Handle_Geom_SurfaceOfLinearExtrusion&);
    virtual ~GeomSurfaceOfExtrusion();
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_SurfaceOfLinearExtrusion& c);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_SurfaceOfLinearExtrusion mySurface;
};


// Helper functions for fillet tools
PartExport 
bool find2DLinesIntersection(const Base::Vector3d &orig1, const Base::Vector3d &dir1,
                             const Base::Vector3d &orig2, const Base::Vector3d &dir2,
                             Base::Vector3d &point);
PartExport
bool find2DLinesIntersection(const GeomLineSegment *lineSeg1, const GeomLineSegment *lineSeg2,
                             Base::Vector3d &point);
PartExport
bool findFilletCenter(const GeomLineSegment *lineSeg1, const GeomLineSegment *lineSeg2, double radius,
                      Base::Vector3d &center);
PartExport
bool findFilletCenter(const GeomLineSegment *lineSeg1, const GeomLineSegment *lineSeg2, double radius,
                      const Base::Vector3d& refPnt1, const Base::Vector3d& refPnt2,
                      Base::Vector3d &center);
PartExport
double suggestFilletRadius(const GeomLineSegment *lineSeg1, const GeomLineSegment *lineSeg2,
                           const Base::Vector3d &refPnt1, const Base::Vector3d &refPnt2);
PartExport
GeomArcOfCircle *createFilletGeometry(const GeomLineSegment *lineSeg1, const GeomLineSegment *lineSeg2,
                                      const Base::Vector3d &center, double radius);
}

#endif // PART_GEOMETRY_H
