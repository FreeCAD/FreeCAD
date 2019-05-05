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
#include <bitset>
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
    virtual const Handle(Geom_Geometry)& handle() const = 0;
    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    /// returns a copy of this object having a new randomly generated tag. If you also want to copy the tag, you may use clone() instead.
    /// For creation of geometry with other handles, with or without the same tag, you may use the constructors and the sethandle functions.
    /// The tag of a geometry can be copied to another geometry using the assignTag function.
    virtual Geometry *copy(void) const = 0;
    /// returns a cloned object. A cloned object has the same tag (see getTag) as the original object.
    /// if you want a copy not having the same tag, you can use copy() instead.
    /// If you want a clone with another geometry handle, it is possible to clone an object and then assign another handle or to create an object
    /// via constructor and use assignTag to assign the tag of the other geometry.
    /// If you do not desire to have the same tag, then a copy can be performed by using a constructor (which will generate another tag)
    /// and then, if necessary (e.g. if the constructor did not take a handle as a parameter), set a new handle.
    Geometry *clone(void) const;
    /// construction geometry (means no impact on a later built topo)
    /// Note: In the Sketcher and only for the specific case of a point, it has a special meaning:
    /// a construction point has fixed coordinates for the solver (it has fixed parameters)
    bool Construction;
    /// returns the tag of the geometry object
    boost::uuids::uuid getTag() const;

    long Id;
    std::string Ref;

    enum Flag {
        Defining = 0, // allow an external geometry to build shape
        Frozen = 1, // freeze an external geometry
        Detached = 2, // signal the intentions of detaching the geometry from external reference
        Missing = 3, // geometry with missing external reference
        Sync = 4, // signal the intension to synchronize a frozen geometry
    };
    std::bitset<32> Flags;

    bool testFlag(int flag) const { return Flags.test((size_t)(flag)); }
    void setFlag(int flag, bool v=true) { Flags.set((size_t)(flag),v); }

protected:
    /// create a new tag for the geometry object
    void createNewTag();
    /// copies the tag from the geometry passed as a parameter to this object
    void assignTag(const Part::Geometry *);

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
    GeomPoint(const Handle(Geom_CartesianPoint)&);
    GeomPoint(const Base::Vector3d&);
    virtual ~GeomPoint();
    virtual Geometry *copy(void) const;
    virtual TopoDS_Shape toShape() const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle(Geom_Geometry)& handle() const;
    void setHandle(const Handle(Geom_CartesianPoint)&);

    Base::Vector3d getPoint(void)const;
    void setPoint(const Base::Vector3d&);

private:
    Handle(Geom_CartesianPoint) myPoint;
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
     * \brief toBSpline Converts the curve to a B-spline
     * \param This is the start parameter of the curve
     * \param This is the end parameter of the curve
     * \return a B-spline curve
     */
    GeomBSplineCurve* toBSpline(double first, double last) const;
    /*!
      The default implementation does the same as \ref toBSpline.
      In sub-classes this can be reimplemented to create a real
      NURBS curve and not just an approximation.
     */
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;
    bool tangent(double u, gp_Dir&) const;
    bool tangent(double u, Base::Vector3d& dir) const;
    Base::Vector3d pointAtParameter(double u) const;
    Base::Vector3d firstDerivativeAtParameter(double u) const;
    Base::Vector3d secondDerivativeAtParameter(double u) const;
    bool closestParameter(const Base::Vector3d& point, double &u) const;
    bool closestParameterToBasisCurve(const Base::Vector3d& point, double &u) const;
    double getFirstParameter() const;
    double getLastParameter() const;
    double curvatureAt(double u) const;
    double length(double u, double v) const;
    bool normalAt(double u, Base::Vector3d& dir) const;
    bool intersect(GeomCurve * c,
                   std::vector<std::pair<Base::Vector3d, Base::Vector3d>>& points,
                   double tol = Precision::Confusion()) const;

    void reverse(void);

protected:
    static bool intersect(const Handle(Geom_Curve) c, const Handle(Geom_Curve) c2,
                          std::vector<std::pair<Base::Vector3d, Base::Vector3d>>& points,
                          double tol = Precision::Confusion());
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
    GeomBezierCurve(const Handle(Geom_BezierCurve)&);
    GeomBezierCurve(const std::vector<Base::Vector3d>&, const std::vector<double>&);
    virtual ~GeomBezierCurve();
    virtual Geometry *copy(void) const;
    std::vector<Base::Vector3d> getPoles() const;
    std::vector<double> getWeights() const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize (void) const;
    virtual void Save (Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_BezierCurve)&);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_BezierCurve) myCurve;
};

class PartExport GeomBSplineCurve : public GeomBoundedCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomBSplineCurve();
    GeomBSplineCurve(const Handle(Geom_BSplineCurve)&);

    GeomBSplineCurve( const std::vector<Base::Vector3d>& poles, const std::vector<double>& weights,
                      const std::vector<double>& knots, const std::vector<int>& multiplicities,
                      int degree, bool periodic=false, bool checkrational = true);

    virtual ~GeomBSplineCurve();
    virtual Geometry *copy(void) const;

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
    bool join(const Handle(Geom_BSplineCurve)&);
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

    void setHandle(const Handle(Geom_BSplineCurve)&);
    const Handle(Geom_Geometry)& handle() const;

private:
    void createArcs(double tolerance, std::list<Geometry*>& new_spans,
                    const gp_Pnt &p_start, const gp_Vec &v_start,
                    double t_start, double t_end, gp_Pnt &p_end, gp_Vec &v_end) const;
    bool calculateBiArcPoints(const gp_Pnt& p0, gp_Vec v_start,
                              const gp_Pnt& p4, gp_Vec v_end,
                              gp_Pnt& p1, gp_Pnt& p2, gp_Pnt& p3) const;
private:
    Handle(Geom_BSplineCurve) myCurve;
};

class PartExport GeomConic : public GeomCurve
{
    TYPESYSTEM_HEADER();

protected:
    GeomConic();

public:
    virtual ~GeomConic();
    virtual Geometry *copy(void) const = 0;

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

    const Handle(Geom_Geometry)& handle() const = 0;
};

class PartExport GeomTrimmedCurve : public GeomBoundedCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomTrimmedCurve();
    GeomTrimmedCurve(const Handle(Geom_TrimmedCurve)&);
    virtual ~GeomTrimmedCurve();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    const Handle(Geom_Geometry)& handle() const;

    bool intersectBasisCurves(  const GeomTrimmedCurve * c,
                            std::vector<std::pair<Base::Vector3d, Base::Vector3d>>& points,
                            double tol = Precision::Confusion()) const;

    virtual void getRange(double& u, double& v) const;
    virtual void setRange(double u, double v);

protected:
    Handle(Geom_TrimmedCurve) myCurve;
};



class PartExport GeomArcOfConic : public GeomTrimmedCurve
{
    TYPESYSTEM_HEADER();

protected:
    GeomArcOfConic();

public:
    virtual ~GeomArcOfConic();
    virtual Geometry *copy(void) const = 0;

    Base::Vector3d getStartPoint(bool emulateCCWXY) const;
    Base::Vector3d getEndPoint(bool emulateCCWXY) const;

    inline virtual Base::Vector3d getStartPoint() const {return getStartPoint(false);}
    inline virtual Base::Vector3d getEndPoint() const {return getEndPoint(false);}
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

    inline virtual void getRange(double& u, double& v) const { getRange(u,v,false);}
    inline virtual void setRange(double u, double v) { setRange(u,v,false);}

    bool isReversed() const;
    double getAngleXU(void) const;
    void setAngleXU(double angle);

    Base::Vector3d getXAxisDir() const;
    void setXAxisDir(const Base::Vector3d& newdir);

    virtual unsigned int getMemSize(void) const = 0;
    virtual PyObject *getPyObject(void) = 0;

    const Handle(Geom_Geometry)& handle() const = 0;
};

class PartExport GeomCircle : public GeomConic
{
    TYPESYSTEM_HEADER();
public:
    GeomCircle();
    GeomCircle(const Handle(Geom_Circle)&);
    virtual ~GeomCircle();
    virtual Geometry *copy(void) const;

    double getRadius(void) const;
    void setRadius(double Radius);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;

    const Handle(Geom_Geometry)& handle() const;

    void setHandle(const Handle(Geom_Circle)&);

private:
    Handle(Geom_Circle) myCurve;
};

class PartExport GeomArcOfCircle : public GeomArcOfConic
{
    TYPESYSTEM_HEADER();
public:
    GeomArcOfCircle();
    GeomArcOfCircle(const Handle(Geom_Circle)&);
    virtual ~GeomArcOfCircle();
    virtual Geometry *copy(void) const;

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

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    void setHandle(const Handle(Geom_Circle)&);
    const Handle(Geom_Geometry)& handle() const;

};

class PartExport GeomEllipse : public GeomConic
{
    TYPESYSTEM_HEADER();
public:
    GeomEllipse();
    GeomEllipse(const Handle(Geom_Ellipse)&);
    virtual ~GeomEllipse();
    virtual Geometry *copy(void) const;

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

    void setHandle(const Handle(Geom_Ellipse) &e);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_Ellipse) myCurve;
};

class PartExport GeomArcOfEllipse : public GeomArcOfConic
{
    TYPESYSTEM_HEADER();
public:
    GeomArcOfEllipse();
    GeomArcOfEllipse(const Handle(Geom_Ellipse)&);
    virtual ~GeomArcOfEllipse();
    virtual Geometry *copy(void) const;

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

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    void setHandle(const Handle(Geom_Ellipse)&);
    const Handle(Geom_Geometry)& handle() const;
};


class PartExport GeomHyperbola : public GeomConic
{
    TYPESYSTEM_HEADER();
public:
    GeomHyperbola();
    GeomHyperbola(const Handle(Geom_Hyperbola)&);
    virtual ~GeomHyperbola();
    virtual Geometry *copy(void) const;

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

    const Handle(Geom_Geometry)& handle() const;
    void setHandle(const Handle(Geom_Hyperbola)&);

private:
    Handle(Geom_Hyperbola) myCurve;
};

class PartExport GeomArcOfHyperbola : public GeomArcOfConic
{
    TYPESYSTEM_HEADER();
public:
    GeomArcOfHyperbola();
    GeomArcOfHyperbola(const Handle(Geom_Hyperbola)&);
    virtual ~GeomArcOfHyperbola();
    virtual Geometry *copy(void) const;

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

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    void setHandle(const Handle(Geom_Hyperbola)&);
    const Handle(Geom_Geometry)& handle() const;
};

class PartExport GeomParabola : public GeomConic
{
    TYPESYSTEM_HEADER();
public:
    GeomParabola();
    GeomParabola(const Handle(Geom_Parabola)&);
    virtual ~GeomParabola();
    virtual Geometry *copy(void) const;

    double getFocal(void) const;
    void setFocal(double length);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);
    virtual GeomBSplineCurve* toNurbs(double first, double last) const;

    const Handle(Geom_Geometry)& handle() const;
    void setHandle(const Handle(Geom_Parabola)&);

private:
    Handle(Geom_Parabola) myCurve;
};

class PartExport GeomArcOfParabola : public GeomArcOfConic
{
    TYPESYSTEM_HEADER();
public:
    GeomArcOfParabola();
    GeomArcOfParabola(const Handle(Geom_Parabola)&);
    virtual ~GeomArcOfParabola();
    virtual Geometry *copy(void) const;

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

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    void setHandle(const Handle(Geom_Parabola)&);
    const Handle(Geom_Geometry)& handle() const;
};

class PartExport GeomLine : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomLine();
    GeomLine(const Handle(Geom_Line)&);
    GeomLine(const Base::Vector3d& Pos, const Base::Vector3d& Dir);
    virtual ~GeomLine();
    virtual Geometry *copy(void) const;

    void setLine(const Base::Vector3d& Pos, const Base::Vector3d& Dir);
    Base::Vector3d getPos(void) const;
    Base::Vector3d getDir(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    const Handle(Geom_Geometry)& handle() const;
    void setHandle(const Handle(Geom_Line)&);

private:
    Handle(Geom_Line) myCurve;
};

class PartExport GeomLineSegment : public GeomTrimmedCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomLineSegment();
    GeomLineSegment(const Handle(Geom_Line)& l);
    virtual ~GeomLineSegment();
    virtual Geometry *copy(void) const;

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

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    void setHandle(const Handle(Geom_Line)&);
    const Handle(Geom_Geometry)& handle() const;

};

class PartExport GeomOffsetCurve : public GeomCurve
{
    TYPESYSTEM_HEADER();
public:
    GeomOffsetCurve();
    GeomOffsetCurve(const Handle(Geom_Curve)&, double, const gp_Dir&);
    GeomOffsetCurve(const Handle(Geom_Curve)&, double, Base::Vector3d&);
    GeomOffsetCurve(const Handle(Geom_OffsetCurve)&);
    virtual ~GeomOffsetCurve();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_OffsetCurve)& c);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_OffsetCurve) myCurve;
};

class PartExport GeomSurface : public Geometry
{
    TYPESYSTEM_HEADER();
public:
    enum Curvature {
        Maximum,
        Minimum,
        Mean,
        Gaussian
    };

    GeomSurface();
    virtual ~GeomSurface();

    TopoDS_Shape toShape() const;
    bool tangentU(double u, double v, gp_Dir& dirU) const;
    bool tangentV(double u, double v, gp_Dir& dirV) const;
    bool normal(double u, double v, gp_Dir& dir) const;

    /** @name Curvature information */
    //@{
    bool isUmbillic(double u, double v) const;
    double curvature(double u, double v, Curvature) const;
    void curvatureDirections(double u, double v, gp_Dir& maxD, gp_Dir& minD) const;
    //@}
};

class PartExport GeomBezierSurface : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomBezierSurface();
    GeomBezierSurface(const Handle(Geom_BezierSurface)&);
    virtual ~GeomBezierSurface();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_BezierSurface)& b);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_BezierSurface) mySurface;
};

class PartExport GeomBSplineSurface : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomBSplineSurface();
    GeomBSplineSurface(const Handle(Geom_BSplineSurface)&);
    virtual ~GeomBSplineSurface();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_BSplineSurface)&);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_BSplineSurface) mySurface;
};

class PartExport GeomCylinder : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomCylinder();
    GeomCylinder(const Handle(Geom_CylindricalSurface)&);
    virtual ~GeomCylinder();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_CylindricalSurface)&);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_CylindricalSurface) mySurface;
};

class PartExport GeomCone : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomCone();
    GeomCone(const Handle(Geom_ConicalSurface)&);
    virtual ~GeomCone();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_ConicalSurface)&);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_ConicalSurface) mySurface;
};

class PartExport GeomSphere : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomSphere();
    GeomSphere(const Handle(Geom_SphericalSurface)&);
    virtual ~GeomSphere();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_SphericalSurface)&);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_SphericalSurface) mySurface;
};

class PartExport GeomToroid : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomToroid();
    GeomToroid(const Handle(Geom_ToroidalSurface)&);
    virtual ~GeomToroid();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_ToroidalSurface)&);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_ToroidalSurface) mySurface;
};

class PartExport GeomPlane : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomPlane();
    GeomPlane(const Handle(Geom_Plane)&);
    virtual ~GeomPlane();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_Plane)&);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_Plane) mySurface;
};

class PartExport GeomOffsetSurface : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomOffsetSurface();
    GeomOffsetSurface(const Handle(Geom_Surface)&, double);
    GeomOffsetSurface(const Handle(Geom_OffsetSurface)&);
    virtual ~GeomOffsetSurface();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_OffsetSurface)& s);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_OffsetSurface) mySurface;
};

class PartExport GeomPlateSurface : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomPlateSurface();
    GeomPlateSurface(const Handle(Geom_Surface)&, const Plate_Plate&);
    GeomPlateSurface(const GeomPlate_BuildPlateSurface&);
    GeomPlateSurface(const Handle(GeomPlate_Surface)&);
    virtual ~GeomPlateSurface();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(GeomPlate_Surface)& s);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(GeomPlate_Surface) mySurface;
};

class PartExport GeomTrimmedSurface : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomTrimmedSurface();
    GeomTrimmedSurface(const Handle(Geom_RectangularTrimmedSurface)&);
    virtual ~GeomTrimmedSurface();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_RectangularTrimmedSurface)& s);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_RectangularTrimmedSurface) mySurface;
};

class PartExport GeomSurfaceOfRevolution : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomSurfaceOfRevolution();
    GeomSurfaceOfRevolution(const Handle(Geom_Curve)&, const gp_Ax1&);
    GeomSurfaceOfRevolution(const Handle(Geom_SurfaceOfRevolution)&);
    virtual ~GeomSurfaceOfRevolution();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_SurfaceOfRevolution)& c);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_SurfaceOfRevolution) mySurface;
};

class PartExport GeomSurfaceOfExtrusion : public GeomSurface
{
    TYPESYSTEM_HEADER();
public:
    GeomSurfaceOfExtrusion();
    GeomSurfaceOfExtrusion(const Handle(Geom_Curve)&, const gp_Dir&);
    GeomSurfaceOfExtrusion(const Handle(Geom_SurfaceOfLinearExtrusion)&);
    virtual ~GeomSurfaceOfExtrusion();
    virtual Geometry *copy(void) const;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    // Base implementer ----------------------------
    virtual PyObject *getPyObject(void);

    void setHandle(const Handle(Geom_SurfaceOfLinearExtrusion)& c);
    const Handle(Geom_Geometry)& handle() const;

private:
    Handle(Geom_SurfaceOfLinearExtrusion) mySurface;
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
GeomSurface *makeFromSurface(const Handle(Geom_Surface)&);
}

#endif // PART_GEOMETRY_H
