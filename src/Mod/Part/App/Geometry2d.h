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

#ifndef PART_GEOMETRY2D_H
#define PART_GEOMETRY2D_H

#include <Adaptor2d_Curve2d.hxx>
#include <Geom2d_BezierCurve.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_CartesianPoint.hxx>
#include <Geom2d_Circle.hxx>
#include <Geom2d_Ellipse.hxx>
#include <Geom2d_Hyperbola.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_OffsetCurve.hxx>
#include <Geom2d_Parabola.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <gp_Ax22d.hxx>
#include <TopoDS_Shape.hxx>

#include <list>
#include <memory>
#include <vector>

#include <Base/Persistence.h>
#include <Base/Tools2D.h>
#include <Mod/Part/PartGlobal.h>


namespace Part {

class PartExport Geometry2d : public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    ~Geometry2d() override;

    virtual TopoDS_Shape toShape() const = 0;
    virtual const Handle(Geom2d_Geometry)& handle() const = 0;
    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    /// returns a cloned object
    virtual Geometry2d *clone() const = 0;

protected:
    Geometry2d();

public:
    Geometry2d(const Geometry2d&) = delete;
    Geometry2d& operator = (const Geometry2d&) = delete;
};

class PartExport Geom2dPoint : public Geometry2d
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dPoint();
    Geom2dPoint(const Handle(Geom2d_CartesianPoint)&);
    Geom2dPoint(const Base::Vector2d&);
    ~Geom2dPoint() override;
    Geometry2d *clone() const override;
    TopoDS_Shape toShape() const override;

   // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    const Handle(Geom2d_Geometry)& handle() const override;

    Base::Vector2d getPoint()const;
    void setPoint(const Base::Vector2d&);

private:
    Handle(Geom2d_CartesianPoint) myPoint;
};

class PartExport Geom2dCurve : public Geometry2d
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dCurve();
    ~Geom2dCurve() override;

    TopoDS_Shape toShape() const override;
    bool tangent(double u, gp_Dir2d&) const;
    Base::Vector2d pointAtParameter(double u) const;
    Base::Vector2d firstDerivativeAtParameter(double u) const;
    Base::Vector2d secondDerivativeAtParameter(double u) const;
    bool normal(double u, gp_Dir2d& dir) const;
    bool closestParameter(const Base::Vector2d& point, double &u) const;
    bool closestParameterToBasicCurve(const Base::Vector2d& point, double &u) const;
};

class PartExport Geom2dBezierCurve : public Geom2dCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dBezierCurve();
    Geom2dBezierCurve(const Handle(Geom2d_BezierCurve)&);
    ~Geom2dBezierCurve() override;
    Geometry2d *clone() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize () const override;
    void Save (Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom2d_BezierCurve)&);
    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_BezierCurve) myCurve;
};

class PartExport Geom2dBSplineCurve : public Geom2dCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dBSplineCurve();
    Geom2dBSplineCurve(const Handle(Geom2d_BSplineCurve)&);
    ~Geom2dBSplineCurve() override;
    Geometry2d *clone() const override;

    /*!
     * Set the poles and tangents for the cubic Hermite spline
     */
    void interpolate(const std::vector<gp_Pnt2d>&, const std::vector<gp_Vec2d>&);
    /*!
     * Compute the tangents for a Cardinal spline using the
     * the cubic Hermite spline. It uses the method for Cardinal splines.
     */
    void getCardinalSplineTangents(const std::vector<gp_Pnt2d>&,
                                   const std::vector<double>&,
                                   std::vector<gp_Vec2d>&) const;
    /*!
     * Compute the tangents for a Cardinal spline using the
     * the cubic Hermite spline. It uses the method for Cardinal splines.
     * It uses the same parameter for each tangent.
     */
    void getCardinalSplineTangents(const std::vector<gp_Pnt2d>&, double,
                                   std::vector<gp_Vec2d>&) const;

    int countPoles() const;
    void setPole(int index, const Base::Vector2d&, double weight=-1);
    std::vector<Base::Vector2d> getPoles() const;
    bool join(const Handle(Geom2d_BSplineCurve)&);
    void makeC1Continuous(double);
    std::list<Geometry2d*> toBiArcs(double tolerance) const;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom2d_BSplineCurve)&);
    const Handle(Geom2d_Geometry)& handle() const override;

private:
    void createArcs(double tolerance, std::list<Geometry2d*>& new_spans,
                    const gp_Pnt2d &p_start, const gp_Vec2d &v_start,
                    double t_start, double t_end, gp_Pnt2d &p_end, gp_Vec2d &v_end) const;
    bool calculateBiArcPoints(const gp_Pnt2d& p0, gp_Vec2d v_start,
                              const gp_Pnt2d& p4, gp_Vec2d v_end,
                              gp_Pnt2d& p1, gp_Pnt2d& p2, gp_Pnt2d& p3) const;
private:
    Handle(Geom2d_BSplineCurve) myCurve;
};

class PartExport Geom2dConic : public Geom2dCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
protected:
    Geom2dConic();

public:
    ~Geom2dConic() override;
    Geometry2d *clone() const override = 0;

    Base::Vector2d getLocation() const;
    void setLocation(const Base::Vector2d& Center);
    bool isReversed() const;

    unsigned int getMemSize() const override = 0;
    PyObject *getPyObject() override = 0;

    const Handle(Geom2d_Geometry)& handle() const override = 0;

protected:
    void SaveAxis(Base::Writer& writer, const gp_Ax22d&) const;
    void RestoreAxis(Base::XMLReader& reader, gp_Ax22d&);
};

class PartExport Geom2dArcOfConic : public Geom2dCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
protected:
    Geom2dArcOfConic();

public:
    ~Geom2dArcOfConic() override;
    Geometry2d *clone() const override = 0;

    Base::Vector2d getLocation() const;
    void setLocation(const Base::Vector2d& Center);
    bool isReversed() const;

    Base::Vector2d getStartPoint() const;
    Base::Vector2d getEndPoint() const;

    void getRange(double& u, double& v) const;
    void setRange(double u, double v);

    unsigned int getMemSize() const override = 0;
    PyObject *getPyObject() override = 0;

    const Handle(Geom2d_Geometry)& handle() const override = 0;

protected:
    void SaveAxis(Base::Writer& writer, const gp_Ax22d&, double u, double v) const;
    void RestoreAxis(Base::XMLReader& reader, gp_Ax22d&, double& u, double& v);
};

class PartExport Geom2dCircle : public Geom2dConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dCircle();
    Geom2dCircle(const Handle(Geom2d_Circle)&);
    ~Geom2dCircle() override;
    Geometry2d *clone() const override;

    double getRadius() const;
    void setRadius(double Radius);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    const Handle(Geom2d_Geometry)& handle() const override;

    static Base::Vector2d getCircleCenter (const Base::Vector2d &p1, const Base::Vector2d &p2, const Base::Vector2d &p3);

private:
    Handle(Geom2d_Circle) myCurve;
};

class PartExport Geom2dArcOfCircle : public Geom2dArcOfConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dArcOfCircle();
    Geom2dArcOfCircle(const Handle(Geom2d_Circle)&);
    ~Geom2dArcOfCircle() override;
    Geometry2d *clone() const override;

    double getRadius() const;
    void setRadius(double Radius);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom2d_TrimmedCurve)&);
    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_TrimmedCurve) myCurve;
};

class PartExport Geom2dEllipse : public Geom2dConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dEllipse();
    Geom2dEllipse(const Handle(Geom2d_Ellipse)&);
    ~Geom2dEllipse() override;
    Geometry2d *clone() const override;

    double getMajorRadius() const;
    void setMajorRadius(double Radius);
    double getMinorRadius() const;
    void setMinorRadius(double Radius);
    Base::Vector2d getMajorAxisDir() const;
    void setMajorAxisDir(Base::Vector2d newdir);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom2d_Ellipse) &e);
    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_Ellipse) myCurve;
};

class PartExport Geom2dArcOfEllipse : public Geom2dArcOfConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dArcOfEllipse();
    Geom2dArcOfEllipse(const Handle(Geom2d_Ellipse)&);
    ~Geom2dArcOfEllipse() override;
    Geometry2d *clone() const override;

    double getMajorRadius() const;
    void setMajorRadius(double Radius);
    double getMinorRadius() const;
    void setMinorRadius(double Radius);
    Base::Vector2d getMajorAxisDir() const;
    void setMajorAxisDir(Base::Vector2d newdir);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom2d_TrimmedCurve)&);
    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_TrimmedCurve) myCurve;
};

class PartExport Geom2dHyperbola : public Geom2dConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dHyperbola();
    Geom2dHyperbola(const Handle(Geom2d_Hyperbola)&);
    ~Geom2dHyperbola() override;
    Geometry2d *clone() const override;

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

    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_Hyperbola) myCurve;
};

class PartExport Geom2dArcOfHyperbola : public Geom2dArcOfConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dArcOfHyperbola();
    Geom2dArcOfHyperbola(const Handle(Geom2d_Hyperbola)&);
    ~Geom2dArcOfHyperbola() override;
    Geometry2d *clone() const override;

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

    void setHandle(const Handle(Geom2d_TrimmedCurve)&);
    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_TrimmedCurve) myCurve;
};

class PartExport Geom2dParabola : public Geom2dConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dParabola();
    Geom2dParabola(const Handle(Geom2d_Parabola)&);
    ~Geom2dParabola() override;
    Geometry2d *clone() const override;

    double getFocal() const;
    void setFocal(double length);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_Parabola) myCurve;
};

class PartExport Geom2dArcOfParabola : public Geom2dArcOfConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dArcOfParabola();
    Geom2dArcOfParabola(const Handle(Geom2d_Parabola)&);
    ~Geom2dArcOfParabola() override;
    Geometry2d *clone() const override;

    double getFocal() const;
    void setFocal(double length);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom2d_TrimmedCurve)&);
    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_TrimmedCurve) myCurve;
};

class PartExport Geom2dLine : public Geom2dCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dLine();
    Geom2dLine(const Handle(Geom2d_Line)&);
    Geom2dLine(const Base::Vector2d& Pos, const Base::Vector2d& Dir);
    ~Geom2dLine() override;
    Geometry2d *clone() const override;

    void setLine(const Base::Vector2d& Pos, const Base::Vector2d& Dir);
    Base::Vector2d getPos() const;
    Base::Vector2d getDir() const;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_Line) myCurve;
};

class PartExport Geom2dLineSegment : public Geom2dCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dLineSegment();
    ~Geom2dLineSegment() override;
    Geometry2d *clone() const override;

    Base::Vector2d getStartPoint() const;
    Base::Vector2d getEndPoint() const;

    void setPoints(const Base::Vector2d& p1,
                   const Base::Vector2d& p2);

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom2d_TrimmedCurve)&);
    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_TrimmedCurve) myCurve;
};

class PartExport Geom2dOffsetCurve : public Geom2dCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dOffsetCurve();
    Geom2dOffsetCurve(const Handle(Geom2d_Curve)&, double);
    Geom2dOffsetCurve(const Handle(Geom2d_OffsetCurve)&);
    ~Geom2dOffsetCurve() override;
    Geometry2d *clone() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom2d_OffsetCurve)& c);
    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_OffsetCurve) myCurve;
};

class PartExport Geom2dTrimmedCurve : public Geom2dCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    Geom2dTrimmedCurve();
    explicit Geom2dTrimmedCurve(const Handle(Geom2d_TrimmedCurve)&);
    ~Geom2dTrimmedCurve() override;
    Geometry2d *clone() const override;

    // Persistence implementer ---------------------
    unsigned int getMemSize() const override;
    void Save(Base::Writer &/*writer*/) const override;
    void Restore(Base::XMLReader &/*reader*/) override;
    // Base implementer ----------------------------
    PyObject *getPyObject() override;

    void setHandle(const Handle(Geom2d_TrimmedCurve)&);
    const Handle(Geom2d_Geometry)& handle() const override;

private:
    Handle(Geom2d_TrimmedCurve) myCurve;
};

std::unique_ptr<Geom2dCurve> makeFromCurve2d(Handle(Geom2d_Curve));
std::unique_ptr<Geom2dCurve> makeFromTrimmedCurve2d(const Handle(Geom2d_Curve)&, double f, double l);
std::unique_ptr<Geom2dCurve> makeFromCurveAdaptor2d(const Adaptor2d_Curve2d&);

}

#endif // PART_GEOMETRY2D_H
