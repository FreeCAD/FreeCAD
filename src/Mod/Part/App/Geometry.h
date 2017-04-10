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

#include <Geom_CartesianPoint.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Hyperbola.hxx>
#include <Geom_Parabola.hxx>
#include <Geom_Line.hxx>
#include <Geom_OffsetCurve.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Geom_Surface.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_OffsetSurface.hxx>
#include <GeomPlate_Surface.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_SurfaceOfLinearExtrusion.hxx>
#include <GeomPlate_BuildPlateSurface.hxx>
#include <Plate_Plate.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <list>
#include <vector>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

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

    virtual Geometry *copy(void) const = 0;
    /// returns a cloned object. A cloned object has the same tag (see getTag) as the original object.
    /// If you want a clone with another handle, it is possible to clone an object and then assign another handle.
    /// If you do not desire to have the same tag, then a copy can be performed by using a constructor (which will generate another tag)
    /// and then, if necessary (e.g. if the constructor did not take a handle as a parameter), set a new handle.
    virtual Geometry *clone(void) const = 0;
    /// construction geometry (means no impact on a later built topo)
    /// Note: In the Sketcher and only for the specific case of a point, it has a special meaning:
    /// a construction point has fixed coordinates for the solver (it has fixed parameters)
    bool Construction;
    /// returns the tag of the geometry object
    boost::uuids::uuid getTag() const;
    /// create a new tag for the geometry object
    void createNewTag();

protected:
    Geometry();
    
protected:
    boost::uuids::uuid tag;    

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
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;
    virtual TopoDS_Shape toShape() const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle_Geom_Geometry& handle() const;
    void setHandle(const Handle_Geom_CartesianPoint&);

    Base::Vector3d getPoint(void)const;
    void setPoint(const Base::Vector3d&);

private:
    Handle_Geom_CartesianPoint myPoint;
};

class GeomBSplineCurve;
class PartExport GeomCurve : public Geometry
{
    TYPESYSTEM_HEADER();
public:
    GeomCurve();
    virtual ~GeomCurve();

    TopoDS_Shape toShape() const;
    /*!
     * \brief toBSpline Converts the curve to a B-Spline
     * \param This is the start parameter of the curve
     * \param This is the end parameter of the curve
     * \return a B-Spline curve
     */
    GeomBSplineCurve* toBSpline(double first, double last) const;
    /*!
      The default implementation does the same as \ref toBSpline.
      In sub-classes this can be reimplemented to create a real
      NURBS curve and not just an approximation.
     */
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;
    bool tangent(double u, gp_Dir&) const;
    Base::Vector3d pointAtParameter(double u) const;
    Base::Vector3d firstDerivativeAtParameter(double u) const;
    Base::Vector3d secondDerivativeAtParameter(double u) const;
    bool closestParameter(const Base::Vector3d& point, double &u) const;
    bool closestParameterToBasicCurve(const Base::Vector3d& point, double &u) const;
    double getFirstParameter() const;
    double getLastParameter() const;
    double curvatureAt(double u) const;
    double length(double u, double v) const;
    bool normalAt(double u, Base::Vector3d& dir) const;
    
    void reverse(void);
};

class PartExport GeomBoundedCurve : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomBoundedCurve();
    virtual ~GeomBoundedCurve();

    // Geometry helper
    virtual Base::Vector3d getStartPoint() const;
    virtual Base::Vector3d getEndPoint() const;
};

class PartExport GeomBezierCurve : public GeomBoundedCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomBezierCurve();
    GeomBezierCurve(const Handle_Geom_BezierCurve&);
    virtual ~GeomBezierCurve();
    virtual Geometry *copy(void) const;
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

class PartExport GeomBSplineCurve : public GeomBoundedCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomBSplineCurve();
    GeomBSplineCurve(const Handle_Geom_BSplineCurve&);
    
    GeomBSplineCurve( const std::vector<Base::Vector3d>& poles, const std::vector<double>& weights,
                      const std::vector<double>& knots, const std::vector<int>& multiplicities,
                      int degree, bool periodic=false, bool checkrational = true);
    
    virtual ~GeomBSplineCurve();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    /*!
     * Set the poles and tangents for the cubic Hermite spline
     */
    void interpolate(const std::vector<gp_Pnt>&, const std::vector<gp_Vec>&);
    /*!
     * Compute the tangents for a Cardinal spline using the
     * the cubic Hermite spline. It uses the method for Cardinal splines.
     */
    void getCardinalSplineTangents(const std::vector<gp_Pnt>&,
                                   const std::vector<double>&,
                                   std::vector<gp_Vec>&) const;
    /*!
     * Compute the tangents for a Cardinal spline using the
     * the cubic Hermite spline. It uses the method for Cardinal splines.
     * It uses the same parameter for each tangent.
     */
    void getCardinalSplineTangents(const std::vector<gp_Pnt>&, double,
                                   std::vector<gp_Vec>&) const;

    int countPoles() const;
    int countKnots() const;
    void setPole(int index, const Base::Vector3d&, double weight=-1);
    void setPoles(const std::vector<Base::Vector3d>& poles, const std::vector<double>& weights);
    void setPoles(const std::vector<Base::Vector3d>& poles);
    void setWeights(const std::vector<double>& weights);
    void setKnot(int index, const double val, int mult=-1);
    void setKnots(const std::vector<double>& knots);
    void setKnots(const std::vector<double>& knots, const std::vector<int>& multiplicities);
    std::vector<Base::Vector3d> getPoles() const;
    std::vector<double> getWeights() const;
    std::vector<double> getKnots() const;
    std::vector<int> getMultiplicities() const;
    int getMultiplicity(int index) const;
    int getDegree() const;
    bool isPeriodic() const;
    bool join(const Handle_Geom_BSplineCurve&);
    void makeC1Continuous(double, double);
    std::list<Geometry*> toBiArcs(double tolerance) const;

    void increaseDegree(double degree);

    void increaseMultiplicity(int index, int multiplicity);
    bool removeKnot(int index, int multiplicity, double tolerance = Precision::PConfusion());

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_BSplineCurve&);
    const Handle_Geom_Geometry& handle() const;

private:
    void createArcs(double tolerance, std::list<Geometry*>& new_spans,
                    const gp_Pnt &p_start, const gp_Vec &v_start,
                    double t_start, double t_end, gp_Pnt &p_end, gp_Vec &v_end) const;
    bool calculateBiArcPoints(const gp_Pnt& p0, gp_Vec v_start,
                              const gp_Pnt& p4, gp_Vec v_end,
                              gp_Pnt& p1, gp_Pnt& p2, gp_Pnt& p3) const;
private:
    Handle_Geom_BSplineCurve myCurve;
};

class PartExport GeomConic : public GeomCurve
{
    TYPESYSTEM_HEADER();

protected:
    GeomConic();

public:
    virtual ~GeomConic();
    virtual Geometry *copy(void) const = 0;
    virtual Geometry *clone(void) const = 0;

    /*!
     * \deprecated use getLocation
     * \brief getCenter
     */
    Base::Vector3d getCenter(void) const;
    Base::Vector3d getLocation(void) const;
    void setLocation(const Base::Vector3d& Center);
    /*!
     * \deprecated use setLocation
     * \brief setCenter
     */
    void setCenter(const Base::Vector3d& Center);
    double getAngleXU(void) const;
    void setAngleXU(double angle);
    bool isReversed() const;

    virtual unsigned int getMemSize(void) const = 0;
    virtual PyObject *getPyObject(void) = 0;

    const Handle_Geom_Geometry& handle() const = 0;
};

class PartExport GeomArcOfConic : public GeomCurve
{
    TYPESYSTEM_HEADER();

protected:
    GeomArcOfConic();

public:
    virtual ~GeomArcOfConic();
    virtual Geometry *copy(void) const = 0;
    virtual Geometry *clone(void) const = 0;

    Base::Vector3d getStartPoint(bool emulateCCWXY=false) const;
    Base::Vector3d getEndPoint(bool emulateCCWXY=false) const;

    /*!
     * \deprecated use getLocation
     * \brief getCenter
     */
    Base::Vector3d getCenter(void) const;
    Base::Vector3d getLocation(void) const;
    void setLocation(const Base::Vector3d& Center);
    /*!
     * \deprecated use setLocation
     * \brief setCenter
     */
    void setCenter(const Base::Vector3d& Center);

    virtual void getRange(double& u, double& v, bool emulateCCWXY) const = 0;
    virtual void setRange(double u, double v, bool emulateCCWXY) = 0;

    bool isReversed() const;
    double getAngleXU(void) const;
    void setAngleXU(double angle);

    Base::Vector3d getXAxisDir() const;
    void setXAxisDir(const Base::Vector3d& newdir);

    virtual unsigned int getMemSize(void) const = 0;
    virtual PyObject *getPyObject(void) = 0;

    const Handle_Geom_Geometry& handle() const = 0;
};

class PartExport GeomCircle : public GeomConic
{
    TYPESYSTEM_HEADER();
public:
    GeomCircle();
    GeomCircle(const Handle_Geom_Circle&);
    virtual ~GeomCircle();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    double getRadius(void) const;
    void setRadius(double Radius);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;

    const Handle_Geom_Geometry& handle() const;
    
    void setHandle(const Handle_Geom_Circle&);

private:
    Handle_Geom_Circle myCurve;
};

class PartExport GeomArcOfCircle : public GeomArcOfConic
{
    TYPESYSTEM_HEADER();
public:
    GeomArcOfCircle();
    GeomArcOfCircle(const Handle_Geom_Circle&);
    virtual ~GeomArcOfCircle();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    double getRadius(void) const;
    void setRadius(double Radius);

    virtual void getRange(double& u, double& v, bool emulateCCWXY) const;
    virtual void setRange(double u, double v, bool emulateCCWXY);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;

    void setHandle(const Handle_Geom_TrimmedCurve&);
    void setHandle(const Handle_Geom_Circle&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_TrimmedCurve myCurve;
};

class PartExport GeomEllipse : public GeomConic
{
    TYPESYSTEM_HEADER();
public:
    GeomEllipse();
    GeomEllipse(const Handle_Geom_Ellipse&);
    virtual ~GeomEllipse();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    double getMajorRadius(void) const;
    void setMajorRadius(double Radius);
    double getMinorRadius(void) const;
    void setMinorRadius(double Radius);
    Base::Vector3d getMajorAxisDir() const;
    void setMajorAxisDir(Base::Vector3d newdir);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;

    void setHandle(const Handle_Geom_Ellipse &e);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_Ellipse myCurve;
};

class PartExport GeomArcOfEllipse : public GeomArcOfConic
{
    TYPESYSTEM_HEADER();
public:
    GeomArcOfEllipse();
    GeomArcOfEllipse(const Handle_Geom_Ellipse&);
    virtual ~GeomArcOfEllipse();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    double getMajorRadius(void) const;
    void setMajorRadius(double Radius);
    double getMinorRadius(void) const;
    void setMinorRadius(double Radius);
    Base::Vector3d getMajorAxisDir() const;
    void setMajorAxisDir(Base::Vector3d newdir);

    virtual void getRange(double& u, double& v, bool emulateCCWXY) const;
    virtual void setRange(double u, double v, bool emulateCCWXY);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;

    void setHandle(const Handle_Geom_TrimmedCurve&);
    void setHandle(const Handle_Geom_Ellipse&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_TrimmedCurve myCurve;
};


class PartExport GeomHyperbola : public GeomConic
{
    TYPESYSTEM_HEADER();
public:
    GeomHyperbola();
    GeomHyperbola(const Handle_Geom_Hyperbola&);
    virtual ~GeomHyperbola();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;
    
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
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;

    const Handle_Geom_Geometry& handle() const;
    void setHandle(const Handle_Geom_Hyperbola&);

private:
    Handle_Geom_Hyperbola myCurve;
};

class PartExport GeomArcOfHyperbola : public GeomArcOfConic
{
    TYPESYSTEM_HEADER();
public:
    GeomArcOfHyperbola();
    GeomArcOfHyperbola(const Handle_Geom_Hyperbola&);
    virtual ~GeomArcOfHyperbola();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    double getMajorRadius(void) const;
    void setMajorRadius(double Radius);
    double getMinorRadius(void) const;
    void setMinorRadius(double Radius);
    Base::Vector3d getMajorAxisDir() const;
    void setMajorAxisDir(Base::Vector3d newdir);

    virtual void getRange(double& u, double& v, bool emulateCCWXY) const;
    virtual void setRange(double u, double v, bool emulateCCWXY);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;

    void setHandle(const Handle_Geom_TrimmedCurve&);
    void setHandle(const Handle_Geom_Hyperbola&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_TrimmedCurve myCurve;
};

class PartExport GeomParabola : public GeomConic
{
    TYPESYSTEM_HEADER();
public:
    GeomParabola();
    GeomParabola(const Handle_Geom_Parabola&);
    virtual ~GeomParabola();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;
    
    double getFocal(void) const;
    void setFocal(double length);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;

    const Handle_Geom_Geometry& handle() const;
    void setHandle(const Handle_Geom_Parabola&);

private:
    Handle_Geom_Parabola myCurve;
};

class PartExport GeomArcOfParabola : public GeomArcOfConic
{
    TYPESYSTEM_HEADER();
public:
    GeomArcOfParabola();
    GeomArcOfParabola(const Handle_Geom_Parabola&);
    virtual ~GeomArcOfParabola();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    double getFocal(void) const;
    void setFocal(double length);
    
    Base::Vector3d getFocus(void) const;
    
    virtual void getRange(double& u, double& v, bool emulateCCWXY) const;
    virtual void setRange(double u, double v, bool emulateCCWXY);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;

    void setHandle(const Handle_Geom_TrimmedCurve&);
    void setHandle(const Handle_Geom_Parabola&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_TrimmedCurve myCurve;
};

class PartExport GeomLine : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomLine();
    GeomLine(const Handle_Geom_Line&);
    GeomLine(const Base::Vector3d& Pos, const Base::Vector3d& Dir);
    virtual ~GeomLine();
    virtual Geometry *copy(void) const;
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
    void setHandle(const Handle_Geom_Line&);

private:
    Handle_Geom_Line myCurve;
};

class PartExport GeomLineSegment : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomLineSegment();
    GeomLineSegment(const Handle_Geom_Line& l);
    virtual ~GeomLineSegment();
    virtual Geometry *copy(void) const;
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
    void setHandle(const Handle_Geom_Line&);
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
    virtual Geometry *copy(void) const;
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
    virtual Geometry *copy(void) const;
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
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_BezierSurface& b);
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
    virtual Geometry *copy(void) const;
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
    GeomCylinder(const Handle_Geom_CylindricalSurface&);
    virtual ~GeomCylinder();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_CylindricalSurface&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_CylindricalSurface mySurface;
};

class PartExport GeomCone : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomCone();
    GeomCone(const Handle_Geom_ConicalSurface&);
    virtual ~GeomCone();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_ConicalSurface&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_ConicalSurface mySurface;
};

class PartExport GeomSphere : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomSphere();
    GeomSphere(const Handle_Geom_SphericalSurface&);
    virtual ~GeomSphere();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_SphericalSurface&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_SphericalSurface mySurface;
};

class PartExport GeomToroid : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomToroid();
    GeomToroid(const Handle_Geom_ToroidalSurface&);
    virtual ~GeomToroid();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_ToroidalSurface&);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_Geom_ToroidalSurface mySurface;
};

class PartExport GeomPlane : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomPlane();
    GeomPlane(const Handle_Geom_Plane&);
    virtual ~GeomPlane();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_Geom_Plane&);
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
    virtual Geometry *copy(void) const;
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

class PartExport GeomPlateSurface : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomPlateSurface();
    GeomPlateSurface(const Handle_Geom_Surface&, const Plate_Plate&);
    GeomPlateSurface(const GeomPlate_BuildPlateSurface&);
    GeomPlateSurface(const Handle_GeomPlate_Surface&);
    virtual ~GeomPlateSurface();
    virtual Geometry *copy(void) const;
    virtual Geometry *clone(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle_GeomPlate_Surface& s);
    const Handle_Geom_Geometry& handle() const;

private:
    Handle_GeomPlate_Surface mySurface;
};

class PartExport GeomTrimmedSurface : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomTrimmedSurface();
    GeomTrimmedSurface(const Handle_Geom_RectangularTrimmedSurface&);
    virtual ~GeomTrimmedSurface();
    virtual Geometry *copy(void) const;
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
    virtual Geometry *copy(void) const;
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
    virtual Geometry *copy(void) const;
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
PartExport
GeomSurface *makeFromSurface(const Handle_Geom_Surface&);
}

#endif // PART_GEOMETRY_H
