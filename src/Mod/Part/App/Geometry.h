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

#include <Adaptor3d_Curve.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Geom_Circle.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Hyperbola.hxx>
#include <Geom_Line.hxx>
#include <Geom_OffsetCurve.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_Parabola.hxx>
#include <Geom_Plane.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_SurfaceOfLinearExtrusion.hxx>
#include <Geom_SurfaceOfRevolution.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <GeomPlate_BuildPlateSurface.hxx>
#include <GeomPlate_Surface.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Plate_Plate.hxx>
#include <TopoDS_Shape.hxx>

#include <list>
#include <memory>
#include <vector>

#include <boost/uuid/uuid_generators.hpp>

#include <Base/Matrix.h>
#include <Base/Placement.h>
#include <Base/Persistence.h>
#include <Base/Vector3D.h>
#include <Mod/Part/PartGlobal.h>

#include "GeometryExtension.h"


namespace Part {

class PartExport Geometry: public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    ~Geometry() override;

    virtual TopoDS_Shape toShape() const = 0;
    virtual const Handle(Geom_Geometry)& handle() const = 0;
    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    /// returns a copy of this object having a new randomly generated tag. If you also want to copy the tag, you may use clone() instead.
    /// For creation of geometry with other handles, with or without the same tag, you may use the constructors and the sethandle functions.
    /// The tag of a geometry can be copied to another geometry using the assignTag function.
    virtual Geometry *copy() const = 0;
    /// returns a cloned object. A cloned object has the same tag (see getTag) as the original object.
    /// if you want a copy not having the same tag, you can use copy() instead.
    /// If you want a clone with another geometry handle, it is possible to clone an object and then assign another handle or to create an object
    /// via constructor and use assignTag to assign the tag of the other geometry.
    /// If you do not desire to have the same tag, then a copy can be performed by using a constructor (which will generate another tag)
    /// and then, if necessary (e.g. if the constructor did not take a handle as a parameter), set a new handle.
    Geometry *clone() const;
    /// returns the tag of the geometry object
    boost::uuids::uuid getTag() const;

    std::vector<std::weak_ptr<const GeometryExtension>> getExtensions() const;

    bool hasExtension(const Base::Type & type) const;
    bool hasExtension(const std::string & name) const;
    std::weak_ptr<const GeometryExtension> getExtension(const Base::Type & type) const;
    std::weak_ptr<const GeometryExtension> getExtension(const std::string & name) const;
    std::weak_ptr<GeometryExtension> getExtension(const Base::Type & type);
    std::weak_ptr<GeometryExtension> getExtension(const std::string & name);
    void setExtension(std::unique_ptr<GeometryExtension> &&geo);
    void deleteExtension(const Base::Type & type);
    void deleteExtension(const std::string & name);

    void mirror(const Base::Vector3d& point);
    void mirror(const Base::Vector3d& point, const Base::Vector3d& dir);
    void rotate(const Base::Placement& plm);
    void scale(const Base::Vector3d& vec, double scale);
    void transform(const Base::Matrix4D& mat);
    void translate(const Base::Vector3d& vec);

protected:
    /// create a new tag for the geometry object
    void createNewTag();
    /// copies the tag from the geometry passed as a parameter to this object
    void assignTag(const Part::Geometry *);

    void copyNonTag(const Part::Geometry *);

protected:
    Geometry();

protected:
    boost::uuids::uuid tag;
    std::vector<std::shared_ptr<GeometryExtension>> extensions;

public:
    Geometry(const Geometry&) = delete;
    Geometry& operator = (const Geometry&) = delete;
};

class PartExport GeomPoint : public Geometry
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomPoint();
    GeomPoint(const Handle(Geom_CartesianPoint)&);
    GeomPoint(const Base::Vector3d&);
    ~GeomPoint() override;
    Geometry *copy() const override;
    TopoDS_Shape toShape() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    const Handle(Geom_Geometry)& handle() const override;
    void setHandle(const Handle(Geom_CartesianPoint)&);

    Base::Vector3d getPoint()const;
    void setPoint(const Base::Vector3d&);

private:
    Handle(Geom_CartesianPoint) myPoint;
};

class GeomBSplineCurve;
class PartExport GeomCurve : public Geometry
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomCurve();
    ~GeomCurve() override;

    TopoDS_Shape toShape() const override;
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
    bool normalAt(const Base::Vector3d & curvepoint, Base::Vector3d& dir) const;
    bool intersect(const GeomCurve *c,
                   std::vector<std::pair<Base::Vector3d, Base::Vector3d>>& points,
                   double tol = Precision::Confusion()) const;

    void reverse();

    Base::Vector3d value(double u) const;

protected:
    static bool intersect(const Handle(Geom_Curve) c, const Handle(Geom_Curve) c2,
                          std::vector<std::pair<Base::Vector3d, Base::Vector3d>>& points,
                          double tol = Precision::Confusion());
};

class PartExport GeomBoundedCurve : public GeomCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomBoundedCurve();
    ~GeomBoundedCurve() override;

    // Geometry helper
    virtual Base::Vector3d getStartPoint() const;
    virtual Base::Vector3d getEndPoint() const;
};

class PartExport GeomBezierCurve : public GeomBoundedCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomBezierCurve();
    GeomBezierCurve(const Handle(Geom_BezierCurve)&);
    GeomBezierCurve(const std::vector<Base::Vector3d>&, const std::vector<double>&);
    ~GeomBezierCurve() override;
    Geometry *copy() const override;
    std::vector<Base::Vector3d> getPoles() const;
    std::vector<double> getWeights() const;

    // Persistence implementer ---------------------
    unsigned int getMemSize () const override;
    void Save (Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_BezierCurve)&);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_BezierCurve) myCurve;
};

class PartExport GeomBSplineCurve : public GeomBoundedCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomBSplineCurve();
    GeomBSplineCurve(const Handle(Geom_BSplineCurve)&);

    GeomBSplineCurve( const std::vector<Base::Vector3d>& poles, const std::vector<double>& weights,
                      const std::vector<double>& knots, const std::vector<int>& multiplicities,
                      int degree, bool periodic=false, bool checkrational = true);

    ~GeomBSplineCurve() override;
    Geometry *copy() const override;

   /*!
    * Interpolate a spline passing through the given points without tangency.
    */
    void interpolate(const std::vector<gp_Pnt>&, Standard_Boolean=Standard_False);
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
    void setPeriodic() const;
    bool isRational() const;
    bool join(const Handle(Geom_BoundedCurve)&);
    void makeC1Continuous(double, double);
    std::list<Geometry*> toBiArcs(double tolerance) const;

    void increaseDegree(int degree);
    bool approximate(double tol3d, int maxSegments, int maxDegree, int continuity);

    void increaseMultiplicity(int index, int multiplicity);
    void insertKnot(double param, int multiplicity);
    bool removeKnot(int index, int multiplicity, double tolerance = Precision::PConfusion());

    void Trim(double u, double v);
    void scaleKnotsToBounds(double u0, double u1);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_BSplineCurve)&);
    const Handle(Geom_Geometry)& handle() const override;

private:
    // If during assignment of weights (during the for loop iteratively setting the poles) all weights
    // become (temporarily) equal even though weights does not have equal values
    // OCCT will convert all the weights (the already assigned and those not yet assigned)
    // to 1.0 (nonrational b-splines have 1.0 weights). This may lead to the assignment of wrong
    // of weight values.
    //
    // The work-around is to temporarily set the last weight to be assigned to a value different from
    // the current value and the to-be-assigned value for the weight at position last-to-be-assign but one.
    void workAroundOCCTBug(const std::vector<double>& weights);
private:
    Handle(Geom_BSplineCurve) myCurve;
};

class PartExport GeomConic : public GeomCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

protected:
    GeomConic();

public:
    ~GeomConic() override;
    Geometry *copy() const override = 0;

    /*!
     * \deprecated use getLocation
     * \brief getCenter
     */
    Base::Vector3d getCenter() const;
    Base::Vector3d getLocation() const;
    void setLocation(const Base::Vector3d& Center);
    /*!
     * \deprecated use setLocation
     * \brief setCenter
     */
    void setCenter(const Base::Vector3d& Center);
    double getAngleXU() const;
    void setAngleXU(double angle);
    bool isReversed() const;

    Base::Vector3d getAxisDirection() const;

    unsigned int getMemSize() const override = 0;
    PyObject *getPyObject() override = 0;
    GeomBSplineCurve* toNurbs(double first, double last) const override;

    const Handle(Geom_Geometry)& handle() const override = 0;
};

class PartExport GeomTrimmedCurve : public GeomBoundedCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomTrimmedCurve();
    GeomTrimmedCurve(const Handle(Geom_TrimmedCurve)&);
    ~GeomTrimmedCurve() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    const Handle(Geom_Geometry)& handle() const override;

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
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

protected:
    GeomArcOfConic();

public:
    ~GeomArcOfConic() override;
    Geometry *copy() const override = 0;

    Base::Vector3d getStartPoint(bool emulateCCWXY) const;
    Base::Vector3d getEndPoint(bool emulateCCWXY) const;

    inline Base::Vector3d getStartPoint() const override {return getStartPoint(false);}
    inline Base::Vector3d getEndPoint() const override {return getEndPoint(false);}
    /*!
     * \deprecated use getLocation
     * \brief getCenter
     */
    Base::Vector3d getCenter() const;
    Base::Vector3d getLocation() const;
    void setLocation(const Base::Vector3d& Center);
    /*!
     * \deprecated use setLocation
     * \brief setCenter
     */
    void setCenter(const Base::Vector3d& Center);

    Base::Vector3d getAxisDirection() const;

    virtual void getRange(double& u, double& v, bool emulateCCWXY) const = 0;
    virtual void setRange(double u, double v, bool emulateCCWXY) = 0;

    inline void getRange(double& u, double& v) const override { getRange(u,v,false);}
    inline void setRange(double u, double v) override { setRange(u,v,false);}

    bool isReversed() const;
    double getAngleXU() const;
    void setAngleXU(double angle);

    Base::Vector3d getXAxisDir() const;
    void setXAxisDir(const Base::Vector3d& newdir);

    unsigned int getMemSize() const override = 0;
    PyObject *getPyObject() override = 0;

    const Handle(Geom_Geometry)& handle() const override = 0;
};

class PartExport GeomCircle : public GeomConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomCircle();
    GeomCircle(const Handle(Geom_Circle)&);
    ~GeomCircle() override;
    Geometry *copy() const override;

    double getRadius() const;
    void setRadius(double Radius);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;
    GeomBSplineCurve* toNurbs(double first, double last) const override;

    const Handle(Geom_Geometry)& handle() const override;

    void setHandle(const Handle(Geom_Circle)&);

private:
    Handle(Geom_Circle) myCurve;
};

class PartExport GeomArcOfCircle : public GeomArcOfConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomArcOfCircle();
    GeomArcOfCircle(const Handle(Geom_Circle)&);
    ~GeomArcOfCircle() override;
    Geometry *copy() const override;

    double getRadius() const;
    void setRadius(double Radius);

    double getAngle(bool emulateCCWXY) const;

    void getRange(double& u, double& v, bool emulateCCWXY) const override;
    void setRange(double u, double v, bool emulateCCWXY) override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;
    GeomBSplineCurve* toNurbs(double first, double last) const override;

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    void setHandle(const Handle(Geom_Circle)&);
    const Handle(Geom_Geometry)& handle() const override;

};

class PartExport GeomEllipse : public GeomConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomEllipse();
    GeomEllipse(const Handle(Geom_Ellipse)&);
    ~GeomEllipse() override;
    Geometry *copy() const override;

    double getMajorRadius() const;
    void setMajorRadius(double Radius);
    double getMinorRadius() const;
    void setMinorRadius(double Radius);
    Base::Vector3d getMajorAxisDir() const;
    void setMajorAxisDir(Base::Vector3d newdir);
    Base::Vector3d getMinorAxisDir() const;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;
    GeomBSplineCurve* toNurbs(double first, double last) const override;

    void setHandle(const Handle(Geom_Ellipse) &e);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_Ellipse) myCurve;
};

class PartExport GeomArcOfEllipse : public GeomArcOfConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomArcOfEllipse();
    GeomArcOfEllipse(const Handle(Geom_Ellipse)&);
    ~GeomArcOfEllipse() override;
    Geometry *copy() const override;

    double getMajorRadius() const;
    void setMajorRadius(double Radius);
    double getMinorRadius() const;
    void setMinorRadius(double Radius);
    Base::Vector3d getMajorAxisDir() const;
    void setMajorAxisDir(Base::Vector3d newdir);

    void getRange(double& u, double& v, bool emulateCCWXY) const override;
    void setRange(double u, double v, bool emulateCCWXY) override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;
    GeomBSplineCurve* toNurbs(double first, double last) const override;

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    void setHandle(const Handle(Geom_Ellipse)&);
    const Handle(Geom_Geometry)& handle() const override;
};


class PartExport GeomHyperbola : public GeomConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomHyperbola();
    GeomHyperbola(const Handle(Geom_Hyperbola)&);
    ~GeomHyperbola() override;
    Geometry *copy() const override;

    double getMajorRadius() const;
    void setMajorRadius(double Radius);
    double getMinorRadius() const;
    void setMinorRadius(double Radius);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;
    GeomBSplineCurve* toNurbs(double first, double last) const override;

    const Handle(Geom_Geometry)& handle() const override;
    void setHandle(const Handle(Geom_Hyperbola)&);

private:
    Handle(Geom_Hyperbola) myCurve;
};

class PartExport GeomArcOfHyperbola : public GeomArcOfConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomArcOfHyperbola();
    GeomArcOfHyperbola(const Handle(Geom_Hyperbola)&);
    ~GeomArcOfHyperbola() override;
    Geometry *copy() const override;

    double getMajorRadius() const;
    void setMajorRadius(double Radius);
    double getMinorRadius() const;
    void setMinorRadius(double Radius);
    Base::Vector3d getMajorAxisDir() const;
    void setMajorAxisDir(Base::Vector3d newdir);

    void getRange(double& u, double& v, bool emulateCCWXY) const override;
    void setRange(double u, double v, bool emulateCCWXY) override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;
    GeomBSplineCurve* toNurbs(double first, double last) const override;

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    void setHandle(const Handle(Geom_Hyperbola)&);
    const Handle(Geom_Geometry)& handle() const override;
};

class PartExport GeomParabola : public GeomConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomParabola();
    GeomParabola(const Handle(Geom_Parabola)&);
    ~GeomParabola() override;
    Geometry *copy() const override;

    double getFocal() const;
    void setFocal(double length);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;
    GeomBSplineCurve* toNurbs(double first, double last) const override;

    const Handle(Geom_Geometry)& handle() const override;
    void setHandle(const Handle(Geom_Parabola)&);

private:
    Handle(Geom_Parabola) myCurve;
};

class PartExport GeomArcOfParabola : public GeomArcOfConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomArcOfParabola();
    GeomArcOfParabola(const Handle(Geom_Parabola)&);
    ~GeomArcOfParabola() override;
    Geometry *copy() const override;

    double getFocal() const;
    void setFocal(double length);

    Base::Vector3d getFocus() const;

    void getRange(double& u, double& v, bool emulateCCWXY) const override;
    void setRange(double u, double v, bool emulateCCWXY) override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;
    GeomBSplineCurve* toNurbs(double first, double last) const override;

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    void setHandle(const Handle(Geom_Parabola)&);
    const Handle(Geom_Geometry)& handle() const override;
};

class PartExport GeomLine : public GeomCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomLine();
    GeomLine(const Handle(Geom_Line)&);
    GeomLine(const Base::Vector3d& Pos, const Base::Vector3d& Dir);
    ~GeomLine() override;
    Geometry *copy() const override;

    void setLine(const Base::Vector3d& Pos, const Base::Vector3d& Dir);
    Base::Vector3d getPos() const;
    Base::Vector3d getDir() const;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    const Handle(Geom_Geometry)& handle() const override;
    void setHandle(const Handle(Geom_Line)&);

private:
    Handle(Geom_Line) myCurve;
};

class PartExport GeomLineSegment : public GeomTrimmedCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomLineSegment();
    GeomLineSegment(const Handle(Geom_Line)& l);
    ~GeomLineSegment() override;
    Geometry *copy() const override;

    Base::Vector3d getStartPoint() const override;
    Base::Vector3d getEndPoint() const override;

    void setPoints(const Base::Vector3d& p1,
                   const Base::Vector3d& p2);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_TrimmedCurve)&);
    void setHandle(const Handle(Geom_Line)&);
    const Handle(Geom_Geometry)& handle() const override;

};

class PartExport GeomOffsetCurve : public GeomCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomOffsetCurve();
    GeomOffsetCurve(const Handle(Geom_Curve)&, double, const gp_Dir&);
    GeomOffsetCurve(const Handle(Geom_Curve)&, double, Base::Vector3d&);
    GeomOffsetCurve(const Handle(Geom_OffsetCurve)&);
    ~GeomOffsetCurve() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_OffsetCurve)& c);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_OffsetCurve) myCurve;
};

class PartExport GeomSurface : public Geometry
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    enum Curvature {
        Maximum,
        Minimum,
        Mean,
        Gaussian
    };

    GeomSurface();
    ~GeomSurface() override;

    TopoDS_Shape toShape() const override;
    bool tangentU(double u, double v, gp_Dir& dirU) const;
    bool tangentV(double u, double v, gp_Dir& dirV) const;
    bool normal(double u, double v, gp_Dir& dir) const;
    /*!
      Computes the derivative of order Nu in the direction U and Nv
      in the direction V at the point P(U, V).
     */
    virtual gp_Vec getDN(double u, double v, int Nu, int Nv) const;

    /** @name Curvature information */
    //@{
    bool isUmbillic(double u, double v) const;
    double curvature(double u, double v, Curvature) const;
    void curvatureDirections(double u, double v, gp_Dir& maxD, gp_Dir& minD) const;
    //@}
};

class PartExport GeomBezierSurface : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomBezierSurface();
    GeomBezierSurface(const Handle(Geom_BezierSurface)&);
    ~GeomBezierSurface() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_BezierSurface)& b);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_BezierSurface) mySurface;
};

class PartExport GeomBSplineSurface : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomBSplineSurface();
    GeomBSplineSurface(const Handle(Geom_BSplineSurface)&);
    ~GeomBSplineSurface() override;
    Geometry *copy() const override;

    void scaleKnotsToBounds(double u0, double u1, double v0, double v1);
    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_BSplineSurface)&);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_BSplineSurface) mySurface;
};

class PartExport GeomCylinder : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomCylinder();
    GeomCylinder(const Handle(Geom_CylindricalSurface)&);
    ~GeomCylinder() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_CylindricalSurface)&);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_CylindricalSurface) mySurface;
};

class PartExport GeomCone : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomCone();
    GeomCone(const Handle(Geom_ConicalSurface)&);
    ~GeomCone() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_ConicalSurface)&);
    const Handle(Geom_Geometry)& handle() const override;

    // Overloaded for Geom_ConicalSurface because of an OCC bug
    gp_Vec getDN(double u, double v, int Nu, int Nv) const override;

private:
    Handle(Geom_ConicalSurface) mySurface;
};

class PartExport GeomSphere : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomSphere();
    GeomSphere(const Handle(Geom_SphericalSurface)&);
    ~GeomSphere() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_SphericalSurface)&);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_SphericalSurface) mySurface;
};

class PartExport GeomToroid : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomToroid();
    GeomToroid(const Handle(Geom_ToroidalSurface)&);
    ~GeomToroid() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_ToroidalSurface)&);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_ToroidalSurface) mySurface;
};

class PartExport GeomPlane : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomPlane();
    GeomPlane(const Handle(Geom_Plane)&);
    ~GeomPlane() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_Plane)&);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_Plane) mySurface;
};

class PartExport GeomOffsetSurface : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomOffsetSurface();
    GeomOffsetSurface(const Handle(Geom_Surface)&, double);
    GeomOffsetSurface(const Handle(Geom_OffsetSurface)&);
    ~GeomOffsetSurface() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_OffsetSurface)& s);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_OffsetSurface) mySurface;
};

class PartExport GeomPlateSurface : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomPlateSurface();
    GeomPlateSurface(const Handle(Geom_Surface)&, const Plate_Plate&);
    GeomPlateSurface(const GeomPlate_BuildPlateSurface&);
    GeomPlateSurface(const Handle(GeomPlate_Surface)&);
    ~GeomPlateSurface() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(GeomPlate_Surface)& s);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(GeomPlate_Surface) mySurface;
};

class PartExport GeomTrimmedSurface : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomTrimmedSurface();
    GeomTrimmedSurface(const Handle(Geom_RectangularTrimmedSurface)&);
    ~GeomTrimmedSurface() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_RectangularTrimmedSurface)& s);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_RectangularTrimmedSurface) mySurface;
};

class PartExport GeomSurfaceOfRevolution : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomSurfaceOfRevolution();
    GeomSurfaceOfRevolution(const Handle(Geom_Curve)&, const gp_Ax1&);
    GeomSurfaceOfRevolution(const Handle(Geom_SurfaceOfRevolution)&);
    ~GeomSurfaceOfRevolution() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_SurfaceOfRevolution)& c);
    const Handle(Geom_Geometry)& handle() const override;

private:
    Handle(Geom_SurfaceOfRevolution) mySurface;
};

class PartExport GeomSurfaceOfExtrusion : public GeomSurface
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    GeomSurfaceOfExtrusion();
    GeomSurfaceOfExtrusion(const Handle(Geom_Curve)&, const gp_Dir&);
    explicit GeomSurfaceOfExtrusion(const Handle(Geom_SurfaceOfLinearExtrusion)&);
    ~GeomSurfaceOfExtrusion() override;
    Geometry *copy() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom_SurfaceOfLinearExtrusion)& c);
    const Handle(Geom_Geometry)& handle() const override;

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
std::unique_ptr<GeomSurface> makeFromSurface(const Handle(Geom_Surface)&);

PartExport
std::unique_ptr<GeomCurve> makeFromCurve(const Handle(Geom_Curve)&);

PartExport
std::unique_ptr<GeomCurve> makeFromTrimmedCurve(const Handle(Geom_Curve)&, double f, double l);

PartExport
std::unique_ptr<GeomCurve> makeFromCurveAdaptor(const Adaptor3d_Curve&);
}

#endif // PART_GEOMETRY_H
