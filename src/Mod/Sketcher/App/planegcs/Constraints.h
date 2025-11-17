/***************************************************************************
 *   Copyright (c) 2011 Konstantinos Poulios <logari81@gmail.com>          *
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

#ifndef PLANEGCS_CONSTRAINTS_H
#define PLANEGCS_CONSTRAINTS_H

#include "../../SketcherGlobal.h"
#include "Geo.h"
#include "Mod/Sketcher/App/planegcs/Util.h"

// This enables debugging code intended to extract information to file bug reports against Eigen,
// not for production code
// #define _GCS_EXTRACT_SOLVER_SUBSYSTEM_

#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
# define _PROTECTED_UNLESS_EXTRACT_MODE_ public
#else
# define _PROTECTED_UNLESS_EXTRACT_MODE_ protected
#endif


namespace GCS
{

///////////////////////////////////////
// Constraints
///////////////////////////////////////

enum ConstraintType
{
    None = 0,
    Equal = 1,
    Difference = 2,
    P2PDistance = 3,
    P2PAngle = 4,
    P2LDistance = 5,
    PointOnLine = 6,
    PointOnPerpBisector = 7,
    Parallel = 8,
    Perpendicular = 9,
    L2LAngle = 10,
    MidpointOnLine = 11,
    TangentCircumf = 12,
    PointOnEllipse = 13,
    TangentEllipseLine = 14,
    InternalAlignmentPoint2Ellipse = 15,
    EqualMajorAxesConic = 16,
    EllipticalArcRangeToEndPoints = 17,
    AngleViaPoint = 18,
    Snell = 19,
    CurveValue = 20,
    PointOnHyperbola = 21,
    InternalAlignmentPoint2Hyperbola = 22,
    PointOnParabola = 23,
    EqualFocalDistance = 24,
    EqualLineLength = 25,
    CenterOfGravity = 26,
    WeightedLinearCombination = 27,
    SlopeAtBSplineKnot = 28,
    PointOnBSpline = 29,
    C2CDistance = 30,
    C2LDistance = 31,
    P2CDistance = 32,
    AngleViaPointAndParam = 33,
    AngleViaPointAndTwoParams = 34,
    AngleViaTwoPoints = 35,
    ArcLength = 36,
};

enum InternalAlignmentType
{
    EllipsePositiveMajorX = 0,
    EllipsePositiveMajorY = 1,
    EllipseNegativeMajorX = 2,
    EllipseNegativeMajorY = 3,
    EllipsePositiveMinorX = 4,
    EllipsePositiveMinorY = 5,
    EllipseNegativeMinorX = 6,
    EllipseNegativeMinorY = 7,
    EllipseFocus2X = 8,
    EllipseFocus2Y = 9,
    HyperbolaPositiveMajorX = 10,
    HyperbolaPositiveMajorY = 11,
    HyperbolaNegativeMajorX = 12,
    HyperbolaNegativeMajorY = 13,
    HyperbolaPositiveMinorX = 14,
    HyperbolaPositiveMinorY = 15,
    HyperbolaNegativeMinorX = 16,
    HyperbolaNegativeMinorY = 17
};

class SketcherExport Constraint
{

public:
    enum class Alignment
    {
        NoInternalAlignment,
        InternalAlignment
    };

    _PROTECTED_UNLESS_EXTRACT_MODE_
        : VEC_pD origpvec;  // is used only as a reference for redirecting and reverting pvec
    VEC_Deri pvec;
    double scale;
    int tag;
    // indicates that pvec has changed and saved pointers must be reconstructed (currently used only
    // in AngleViaPoint)
    bool pvecChangedFlag;
    bool driving;
    Alignment internalAlignment;
    ConstraintType typeId {ConstraintType::None};

public:
    Constraint(ConstraintType typeId);
    virtual ~Constraint()
    {}

    VEC_pD origParams() const
    {
        return origpvec;
    }
    VEC_Deri params() const
    {
        return pvec;
    }

    void redirectParams(const UMAP_pD_pD& redirectionmap);

    // Returns false if no parameters in this constraint can be solved
    bool redirectParams(const UMAP_pD_pD& redirectionmap, const UMAP_pD_pD& substitutionmap);
    void revertParams();
    void assignOrigToPvec();
    void setTag(int tagId)
    {
        tag = tagId;
    }
    int getTag()
    {
        return tag;
    }

    void setDriving(bool isdriving)
    {
        driving = isdriving;
    }
    bool isDriving() const
    {
        return driving;
    }

    void setInternalAlignment(Alignment isinternalalignment)
    {
        internalAlignment = isinternalalignment;
    }
    Alignment isInternalAlignment() const
    {
        return internalAlignment;
    }


    ConstraintType getTypeId() const
    {
        return typeId;
    }
    virtual void rescale(double coef = 1.);

    // error and gradient combined. Values are returned through pointers.
    virtual void errorgrad(double* err, double* grad, double* param)
    {
        (void)param;
        if (err) {
            *err = 0.;
        }
        if (grad) {
            *grad = 0.;
        }
    };
    virtual double error()
    {
        double err;
        errorgrad(&err, nullptr, nullptr);
        return scale * err;
    };
    virtual double grad(double* param)
    {
        if (findParamInPvec(param) == -1) {
            return 0.0;
        }

        double deriv;
        errorgrad(nullptr, &deriv, param);

        return deriv * scale;
    };
    // virtual void grad(MAP_pD_D &deriv);  --> TODO: vectorized grad version
    virtual double maxStep(MAP_pD_D& dir, double lim = 1.);
    // Finds first occurrence of param in pvec. This is useful to test if a constraint depends
    // on the parameter (it may not actually depend on it, e.g. angle-via-point doesn't depend
    // on ellipse's b (radmin), but b will be included within the constraint anyway.
    // Returns -1 if not found.
    int findParamInPvec(double* deri);
};

// Equal
class ConstraintEqual: public Constraint
{
public:
    double ratio;
    DeriParam param1() const
    {
        return pvec[0];
    }
    DeriParam param2() const
    {
        return pvec[1];
    }

public:
    ConstraintEqual(double* p1, double* p2, double p1p2ratio = 1.0);

    double error() override;
    double grad(double*) override;
};

// Center of Gravity
class ConstraintCenterOfGravity: public Constraint
{
    DeriParam thecenter() const
    {
        return pvec[0];
    }
    DeriParam pointat(size_t i) const
    {
        return pvec[1 + i];
    }

public:
    /// Constrains that the first parameter is center of gravity of rest
    /// Let `pvec = [q, p_1, p_2,...]`, and
    /// `givenweights = [f_1, f_2,...]`, then this constraint ensures
    /// `q = sum(p_i*f_i)`.
    ConstraintCenterOfGravity(
        const std::vector<double*>& givenpvec,
        const std::vector<double>& givenweights
    );

    double error() override;
    double grad(double*) override;

private:
    std::vector<double> weights;
    std::size_t numpoints;
};

// Weighted Linear Combination
class ConstraintWeightedLinearCombination: public Constraint
{
    DeriParam thepoint() const
    {
        return pvec[0];
    }
    DeriParam poleat(size_t i) const
    {
        return pvec[1 + i];
    }
    DeriParam weightat(size_t i) const
    {
        return pvec[1 + numpoles + i];
    }

public:
    /// Constrains that the first element in pvec is a linear combination
    /// of the next numpoints elements in homogeneous coordinates with
    /// weights given in the last numpoints elements.
    /// Let `pvec = [q, p_1, p_2,... w_1, w_2,...]`, and
    /// `givenfactors = [f_1, f_2,...]`, then this constraint ensures
    /// `q*sum(w_i*f_i) = sum(p_i*w_i*f_i)`.
    ///
    /// This constraint is currently used to ensure that a B-spline knot
    /// point remains at the position determined by it's poles.
    /// In that case, `q` is the x (or y) coordinate of the knot, `p_i` are
    /// the x (or y) coordinates of the poles, and `w_i` are their weights.
    /// Finally, `f_i` are obtained using `BSpline::getLinCombFactor()`.
    ConstraintWeightedLinearCombination(
        size_t givennumpoints,
        const std::vector<double*>& givenpvec,
        const std::vector<double>& givenfactors
    );

    double error() override;
    double grad(double*) override;

private:
    std::vector<double> factors;
    size_t numpoles;
};

// Slope at knot
class ConstraintSlopeAtBSplineKnot: public Constraint
{
private:
    DeriParam polexat(size_t i) const
    {
        return pvec[i];
    }
    DeriParam poleyat(size_t i) const
    {
        return pvec[numpoles + i];
    }
    DeriParam weightat(size_t i) const
    {
        return pvec[2 * numpoles + i];
    }
    DeriParam linep1x() const
    {
        return pvec[3 * numpoles + 0];
    }
    DeriParam linep1y() const
    {
        return pvec[3 * numpoles + 1];
    }
    DeriParam linep2x() const
    {
        return pvec[3 * numpoles + 2];
    }
    DeriParam linep2y() const
    {
        return pvec[3 * numpoles + 3];
    }

public:
    // TODO: Should be able to make the geometries passed const
    // Constrains the slope at a (C1 continuous) knot of the b-spline
    ConstraintSlopeAtBSplineKnot(BSpline& b, Line& l, size_t knotindex);

    void rescale(double coef = 1.) override;
    double error() override;
    double grad(double*) override;

private:
    std::vector<double> factors;
    std::vector<double> slopefactors;
    size_t numpoles;
};

// Point On BSpline
class ConstraintPointOnBSpline: public Constraint
{
private:
    DeriParam thepoint() const
    {
        return pvec[0];
    }
    // TODO: better name because param has a different meaning here?
    DeriParam theparam() const
    {
        return pvec[1];
    }
    DeriParam poleat(size_t i) const
    {
        return pvec[2 + (startpole + i) % bsp.poles.size()];
    }
    DeriParam weightat(size_t i) const
    {
        return pvec[2 + bsp.poles.size() + (startpole + i) % bsp.weights.size()];
    }
    void setStartPole(double u);

public:
    /// TODO: Explain how it's provided
    /// coordidx = 0 if x, 1 if y
    ConstraintPointOnBSpline(double* point, double* initparam, int coordidx, BSpline& b);

    double error() override;
    double grad(double*) override;
    size_t numpoints;
    BSpline& bsp;
    size_t startpole;
};

// Difference
class ConstraintDifference: public Constraint
{
public:
    DeriParam param1() const
    {
        return pvec[0];
    }
    DeriParam param2() const
    {
        return pvec[1];
    }
    DeriParam difference() const
    {
        return pvec[2];
    }

public:
    ConstraintDifference(double* p1, double* p2, double* d);

    double error() override;
    double grad(double*) override;
};

// P2PDistance
class ConstraintP2PDistance: public Constraint
{
private:
    DeriParam p1x() const
    {
        return pvec[0];
    }
    DeriParam p1y() const
    {
        return pvec[1];
    }
    DeriParam p2x() const
    {
        return pvec[2];
    }
    DeriParam p2y() const
    {
        return pvec[3];
    }
    DeriParam distance() const
    {
        return pvec[4];
    }

public:
    ConstraintP2PDistance(Point& p1, Point& p2, double* d);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintP2PDistance()
    {}
#endif

    double error() override;
    double grad(double*) override;
    double maxStep(MAP_pD_D& dir, double lim = 1.) override;
};

// P2PAngle
class ConstraintP2PAngle: public Constraint
{
private:
    DeriParam p1x() const
    {
        return pvec[0];
    }
    DeriParam p1y() const
    {
        return pvec[1];
    }
    DeriParam p2x() const
    {
        return pvec[2];
    }
    DeriParam p2y() const
    {
        return pvec[3];
    }
    DeriParam angle() const
    {
        return pvec[4];
    }
    double da;

public:
    ConstraintP2PAngle(Point& p1, Point& p2, double* a, double da_ = 0.);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintP2PAngle()
    {}
#endif

    double error() override;
    double grad(double*) override;
    double maxStep(MAP_pD_D& dir, double lim = 1.) override;
};

// P2LDistance
class ConstraintP2LDistance: public Constraint
{
private:
    DeriParam p0x() const
    {
        return pvec[0];
    }
    DeriParam p0y() const
    {
        return pvec[1];
    }
    DeriParam p1x() const
    {
        return pvec[2];
    }
    DeriParam p1y() const
    {
        return pvec[3];
    }
    DeriParam p2x() const
    {
        return pvec[4];
    }
    DeriParam p2y() const
    {
        return pvec[5];
    }
    DeriParam distance() const
    {
        return pvec[6];
    }

public:
    ConstraintP2LDistance(Point& p, Line& l, double* d);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintP2LDistance()
    {}
#endif

    double error() override;
    double grad(double*) override;
    double maxStep(MAP_pD_D& dir, double lim = 1.) override;
    double abs(double darea);
};

// PointOnLine
class ConstraintPointOnLine: public Constraint
{
private:
    DeriParam p0x() const
    {
        return pvec[0];
    }
    DeriParam p0y() const
    {
        return pvec[1];
    }
    DeriParam p1x() const
    {
        return pvec[2];
    }
    DeriParam p1y() const
    {
        return pvec[3];
    }
    DeriParam p2x() const
    {
        return pvec[4];
    }
    DeriParam p2y() const
    {
        return pvec[5];
    }

public:
    ConstraintPointOnLine(Point& p, Line& l);
    ConstraintPointOnLine(Point& p, Point& lp1, Point& lp2);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintPointOnLine()
    {}
#endif

    double error() override;
    double grad(double*) override;
};

// PointOnPerpBisector
class ConstraintPointOnPerpBisector: public Constraint
{
private:
    DeriParam p0x() const
    {
        return pvec[0];
    }
    DeriParam p0y() const
    {
        return pvec[1];
    }
    DeriParam p1x() const
    {
        return pvec[2];
    }
    DeriParam p1y() const
    {
        return pvec[3];
    }
    DeriParam p2x() const
    {
        return pvec[4];
    }
    DeriParam p2y() const
    {
        return pvec[5];
    }
    void errorgrad(double* err, double* grad, double* param) override;

public:
    ConstraintPointOnPerpBisector(Point& p, Line& l);
    ConstraintPointOnPerpBisector(Point& p, Point& lp1, Point& lp2);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintPointOnPerpBisector() {};
#endif
};

// Parallel
class ConstraintParallel: public Constraint
{
private:
    DeriParam l1p1x() const
    {
        return pvec[0];
    }
    DeriParam l1p1y() const
    {
        return pvec[1];
    }
    DeriParam l1p2x() const
    {
        return pvec[2];
    }
    DeriParam l1p2y() const
    {
        return pvec[3];
    }
    DeriParam l2p1x() const
    {
        return pvec[4];
    }
    DeriParam l2p1y() const
    {
        return pvec[5];
    }
    DeriParam l2p2x() const
    {
        return pvec[6];
    }
    DeriParam l2p2y() const
    {
        return pvec[7];
    }

public:
    ConstraintParallel(Line& l1, Line& l2);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintParallel()
    {}
#endif

    void rescale(double coef = 1.) override;
    double error() override;
    double grad(double*) override;
};

// Perpendicular
class ConstraintPerpendicular: public Constraint
{
private:
    DeriParam l1p1x() const
    {
        return pvec[0];
    }
    DeriParam l1p1y() const
    {
        return pvec[1];
    }
    DeriParam l1p2x() const
    {
        return pvec[2];
    }
    DeriParam l1p2y() const
    {
        return pvec[3];
    }
    DeriParam l2p1x() const
    {
        return pvec[4];
    }
    DeriParam l2p1y() const
    {
        return pvec[5];
    }
    DeriParam l2p2x() const
    {
        return pvec[6];
    }
    DeriParam l2p2y() const
    {
        return pvec[7];
    }

public:
    ConstraintPerpendicular(Line& l1, Line& l2);
    ConstraintPerpendicular(Point& l1p1, Point& l1p2, Point& l2p1, Point& l2p2);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintPerpendicular()
    {}
#endif

    void rescale(double coef = 1.) override;
    double error() override;
    double grad(double*) override;
};

// L2LAngle
class ConstraintL2LAngle: public Constraint
{
private:
    DeriParam l1p1x() const
    {
        return pvec[0];
    }
    DeriParam l1p1y() const
    {
        return pvec[1];
    }
    DeriParam l1p2x() const
    {
        return pvec[2];
    }
    DeriParam l1p2y() const
    {
        return pvec[3];
    }
    DeriParam l2p1x() const
    {
        return pvec[4];
    }
    DeriParam l2p1y() const
    {
        return pvec[5];
    }
    DeriParam l2p2x() const
    {
        return pvec[6];
    }
    DeriParam l2p2y() const
    {
        return pvec[7];
    }
    DeriParam angle() const
    {
        return pvec[8];
    }

public:
    ConstraintL2LAngle(Line& l1, Line& l2, double* a);
    ConstraintL2LAngle(Point& l1p1, Point& l1p2, Point& l2p1, Point& l2p2, double* a);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintL2LAngle()
    {}
#endif

    double error() override;
    double grad(double*) override;
    double maxStep(MAP_pD_D& dir, double lim = 1.) override;
};

// MidpointOnLine
class ConstraintMidpointOnLine: public Constraint
{
private:
    DeriParam l1p1x() const
    {
        return pvec[0];
    }
    DeriParam l1p1y() const
    {
        return pvec[1];
    }
    DeriParam l1p2x() const
    {
        return pvec[2];
    }
    DeriParam l1p2y() const
    {
        return pvec[3];
    }
    DeriParam l2p1x() const
    {
        return pvec[4];
    }
    DeriParam l2p1y() const
    {
        return pvec[5];
    }
    DeriParam l2p2x() const
    {
        return pvec[6];
    }
    DeriParam l2p2y() const
    {
        return pvec[7];
    }

public:
    ConstraintMidpointOnLine(Line& l1, Line& l2);
    ConstraintMidpointOnLine(Point& l1p1, Point& l1p2, Point& l2p1, Point& l2p2);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintMidpointOnLine()
    {}
#endif

    double error() override;
    double grad(double*) override;
};

// TangentCircumf
class ConstraintTangentCircumf: public Constraint
{
private:
    DeriParam c1x() const
    {
        return pvec[0];
    }
    DeriParam c1y() const
    {
        return pvec[1];
    }
    DeriParam c2x() const
    {
        return pvec[2];
    }
    DeriParam c2y() const
    {
        return pvec[3];
    }
    DeriParam r1() const
    {
        return pvec[4];
    }
    DeriParam r2() const
    {
        return pvec[5];
    }
    bool internal;

public:
    ConstraintTangentCircumf(Point& p1, Point& p2, double* rd1, double* rd2, bool internal_ = false);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintTangentCircumf(bool internal_)
    {
        internal = internal_;
    }
#endif
    bool getInternal()
    {
        return internal;
    };

    double error() override;
    double grad(double*) override;
};
// PointOnEllipse
class ConstraintPointOnEllipse: public Constraint
{
private:
    DeriParam p1x() const
    {
        return pvec[0];
    }
    DeriParam p1y() const
    {
        return pvec[1];
    }
    DeriParam cx() const
    {
        return pvec[2];
    }
    DeriParam cy() const
    {
        return pvec[3];
    }
    DeriParam f1x() const
    {
        return pvec[4];
    }
    DeriParam f1y() const
    {
        return pvec[5];
    }
    DeriParam rmin() const
    {
        return pvec[6];
    }

public:
    ConstraintPointOnEllipse(Point& p, Ellipse& e);
    ConstraintPointOnEllipse(Point& p, ArcOfEllipse& a);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintPointOnEllipse()
    {}
#endif

    double error() override;
    double grad(double*) override;
};

class ConstraintEllipseTangentLine: public Constraint
{
private:
    Line l;
    Ellipse e;
    // writes pointers in pvec to the parameters of crv1, crv2 and poa
    void ReconstructGeomPointers();
    void errorgrad(double* err, double* grad, double* param) override;

public:
    ConstraintEllipseTangentLine(Line& l, Ellipse& e);
};

class ConstraintInternalAlignmentPoint2Ellipse: public Constraint
{
public:
    ConstraintInternalAlignmentPoint2Ellipse(Ellipse& e, Point& p1, InternalAlignmentType alignmentType);


private:
    void errorgrad(double* err, double* grad, double* param) override;
    // writes pointers in pvec to the parameters of crv1, crv2 and poa
    void ReconstructGeomPointers();
    Ellipse e;
    Point p;
    InternalAlignmentType AlignmentType;
};

class ConstraintInternalAlignmentPoint2Hyperbola: public Constraint
{
public:
    ConstraintInternalAlignmentPoint2Hyperbola(
        Hyperbola& e,
        Point& p1,
        InternalAlignmentType alignmentType
    );


private:
    void errorgrad(double* err, double* grad, double* param) override;
    // writes pointers in pvec to the parameters of crv1, crv2 and poa
    void ReconstructGeomPointers();
    Hyperbola e;
    Point p;
    InternalAlignmentType AlignmentType;
};

class ConstraintEqualMajorAxesConic: public Constraint
{
private:
    MajorRadiusConic* e1;
    MajorRadiusConic* e2;
    // writes pointers in pvec to the parameters of crv1, crv2 and poa
    void ReconstructGeomPointers();
    void errorgrad(double* err, double* grad, double* param) override;

public:
    ConstraintEqualMajorAxesConic(MajorRadiusConic* a1, MajorRadiusConic* a2);
};

class ConstraintEqualFocalDistance: public Constraint
{
private:
    ArcOfParabola* e1;
    ArcOfParabola* e2;
    // writes pointers in pvec to the parameters of crv1, crv2 and poa
    void ReconstructGeomPointers();
    void errorgrad(double* err, double* grad, double* param) override;

public:
    ConstraintEqualFocalDistance(ArcOfParabola* a1, ArcOfParabola* a2);
};

class ConstraintCurveValue: public Constraint
{
private:
    // defines, which coordinate of point is being constrained by this constraint
    DeriParam pcoord() const
    {
        return pvec[2];
    }
    DeriParam u() const
    {
        return pvec[3];
    }
    void errorgrad(double* err, double* grad, double* param) override;
    // writes pointers in pvec to the parameters of crv1, crv2 and poa
    void ReconstructGeomPointers();
    Curve* crv;
    Point p;

public:
    /**
     * @brief ConstraintCurveValue: solver constraint that ties parameter value with point
     * coordinates, according to curve's parametric equation.
     * @param p : endpoint to be constrained
     * @param pcoord : pointer to point coordinate to be constrained. Must be either p.x or p.y
     * @param crv : the curve (crv->Value() must be functional)
     * @param u : pointer to u parameter corresponding to the point
     */
    ConstraintCurveValue(Point& p, double* pcoord, Curve& crv, double* u);
    ~ConstraintCurveValue() override;

    double maxStep(MAP_pD_D& dir, double lim = 1.) override;
};

// PointOnHyperbola
class ConstraintPointOnHyperbola: public Constraint
{
private:
    DeriParam p1x() const
    {
        return pvec[0];
    }
    DeriParam p1y() const
    {
        return pvec[1];
    }
    DeriParam cx() const
    {
        return pvec[2];
    }
    DeriParam cy() const
    {
        return pvec[3];
    }
    DeriParam f1x() const
    {
        return pvec[4];
    }
    DeriParam f1y() const
    {
        return pvec[5];
    }
    DeriParam rmin() const
    {
        return pvec[6];
    }

public:
    ConstraintPointOnHyperbola(Point& p, Hyperbola& e);
    ConstraintPointOnHyperbola(Point& p, ArcOfHyperbola& a);
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintPointOnHyperbola()
    {}
#endif

    double error() override;
    double grad(double*) override;
};

// PointOnParabola
class ConstraintPointOnParabola: public Constraint
{
private:
    void errorgrad(double* err, double* grad, double* param) override;
    // writes pointers in pvec to the parameters of crv1, crv2 and poa
    void ReconstructGeomPointers();
    Parabola* parab;
    Point p;

public:
    ConstraintPointOnParabola(Point& p, Parabola& e);
    ConstraintPointOnParabola(Point& p, ArcOfParabola& a);
    ~ConstraintPointOnParabola() override;
#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
    ConstraintPointOnParabola()
    {}
#endif
};

class ConstraintAngleViaPoint: public Constraint
{
private:
    DeriParam angle() const
    {
        return pvec[0];
    };
    Curve* crv1;
    Curve* crv2;
    // These two pointers hold copies of the curves that were passed on
    //  constraint creation. The curves must be deleted upon destruction of
    //  the constraint. It is necessary to have copies, since messing with
    //  original objects that were passed is a very bad idea (but messing is
    //  necessary, because we need to support redirectParams()/revertParams
    //  functions.
    // The pointers in the curves need to be reconstructed if pvec was redirected
    //  (test pvecChangedFlag variable before use!)
    // poa=point of angle //needs to be reconstructed if pvec was redirected/reverted. The point is
    // easily shallow-copied by C++, so no pointer type here and no delete is necessary.
    Point poa;
    // writes pointers in pvec to the parameters of crv1, crv2 and poa
    void ReconstructGeomPointers();

public:
    ConstraintAngleViaPoint(Curve& acrv1, Curve& acrv2, Point p, double* angle);
    ~ConstraintAngleViaPoint() override;

    double error() override;
    double grad(double*) override;
};

class ConstraintAngleViaTwoPoints: public Constraint
{
private:
    DeriParam angle() const
    {
        return pvec[0];
    };
    Curve* crv1;
    Curve* crv2;
    // These two pointers hold copies of the curves that were passed on
    //  constraint creation. The curves must be deleted upon destruction of
    //  the constraint. It is necessary to have copies, since messing with
    //  original objects that were passed is a very bad idea (but messing is
    //  necessary, because we need to support redirectParams()/revertParams
    //  functions.
    // The pointers in the curves need to be reconstructed if pvec was redirected
    //  (test pvecChangedFlag variable before use!)
    // poa=point of angle //needs to be reconstructed if pvec was redirected/reverted. The points
    // are easily shallow-copied by C++, so no pointer type here and no delete is necessary. We use
    // two points in this method as a workaround for B-splines (and friends). There, normals at
    // general points are not implemented, just at their stored start/end points.
    Point poa1;
    Point poa2;
    // writes pointers in pvec to the parameters of crv1, crv2 and poa
    void ReconstructGeomPointers();

public:
    ConstraintAngleViaTwoPoints(Curve& acrv1, Curve& acrv2, Point p1, Point p2, double* angle);
    ~ConstraintAngleViaTwoPoints() override;

    double error() override;
    double grad(double*) override;
};

// snell's law angles constrainer. Point needs to lie on all three curves to be constraied.
class ConstraintSnell: public Constraint
{
private:
    DeriParam n1() const
    {
        return pvec[0];
    };
    DeriParam n2() const
    {
        return pvec[1];
    };
    Curve* ray1;
    Curve* ray2;
    Curve* boundary;
    // These pointers hold copies of the curves that were passed on
    //  constraint creation. The curves must be deleted upon destruction of
    //  the constraint. It is necessary to have copies, since messing with
    //  original objects that were passed is a very bad idea (but messing is
    //  necessary, because we need to support redirectParams()/revertParams
    //  functions.
    // The pointers in the curves need to be reconstructed if pvec was redirected
    //  (test pvecChangedFlag variable before use!)
    // poa=point of refraction //needs to be reconstructed if pvec was redirected/reverted. The
    // point is easily shallow-copied by C++, so no pointer type here and no delete is necessary.
    Point poa;
    bool flipn1, flipn2;
    // writes pointers in pvec to the parameters of crv1, crv2 and poa
    void ReconstructGeomPointers();
    void errorgrad(double* err, double* grad, double* param) override;

public:
    // n1dn2 = n1 divided by n2. from n1 to n2. flipn1 = true instructs to flip ray1's tangent
    ConstraintSnell(
        Curve& ray1,
        Curve& ray2,
        Curve& boundary,
        Point p,
        double* n1,
        double* n2,
        bool flipn1,
        bool flipn2
    );
    ~ConstraintSnell() override;
};

class ConstraintAngleViaPointAndParam: public Constraint
{
private:
    DeriParam angle() const
    {
        return pvec[0];
    };
    DeriParam cparam() const
    {
        return pvec[3];
    };
    Curve* crv1;
    Curve* crv2;
    // These two pointers hold copies of the curves that were passed on
    //  constraint creation. The curves must be deleted upon destruction of
    //  the constraint. It is necessary to have copies, since messing with
    //  original objects that were passed is a very bad idea (but messing is
    //  necessary, because we need to support redirectParams()/revertParams
    //  functions.
    // The pointers in the curves need to be reconstructed if pvec was redirected
    //  (test pvecChangedFlag variable before use!)
    Point poa;  // poa=point of angle //needs to be reconstructed if pvec was redirected/reverted.
                // The point is easily shallow-copied by C++, so no pointer type here and no delete
                // is necessary.
    void ReconstructGeomPointers();  // writes pointers in pvec to the parameters of crv1, crv2 and poa
public:
    // We assume first curve needs param1
    ConstraintAngleViaPointAndParam(Curve& acrv1, Curve& acrv2, Point p, double* param1, double* angle);
    ~ConstraintAngleViaPointAndParam() override;

    double error() override;
    double grad(double*) override;
};

// TODO: Do we need point here at all?
class ConstraintAngleViaPointAndTwoParams: public Constraint
{
private:
    DeriParam angle() const
    {
        return pvec[0];
    };
    DeriParam cparam1() const
    {
        return pvec[3];
    };
    DeriParam cparam2() const
    {
        return pvec[4];
    };
    Curve* crv1;
    Curve* crv2;
    // These two pointers hold copies of the curves that were passed on
    //  constraint creation. The curves must be deleted upon destruction of
    //  the constraint. It is necessary to have copies, since messing with
    //  original objects that were passed is a very bad idea (but messing is
    //  necessary, because we need to support redirectParams()/revertParams
    //  functions.
    // The pointers in the curves need to be reconstructed if pvec was redirected
    //  (test pvecChangedFlag variable before use!)
    Point poa;  // poa=point of angle //needs to be reconstructed if pvec was redirected/reverted.
                // The point is easily shallow-copied by C++, so no pointer type here and no delete
                // is necessary.
    void ReconstructGeomPointers();  // writes pointers in pvec to the parameters of crv1, crv2 and poa
public:
    ConstraintAngleViaPointAndTwoParams(
        Curve& acrv1,
        Curve& acrv2,
        Point p,
        double* param1,
        double* param2,
        double* angle
    );
    ~ConstraintAngleViaPointAndTwoParams() override;

    double error() override;
    double grad(double*) override;
};

class ConstraintEqualLineLength: public Constraint
{
public:
    Line l1;
    Line l2;

private:
    // writes pointers in pvec to the parameters of line1, line2
    void ReconstructGeomPointers();
    void errorgrad(double* err, double* grad, double* param) override;

public:
    ConstraintEqualLineLength(Line& l1, Line& l2);
};

class ConstraintC2CDistance: public Constraint
{
private:
    Circle c1;
    Circle c2;
    DeriParam distance() const
    {
        return pvec[0];
    }
    // writes pointers in pvec to the parameters of c1, c2
    void ReconstructGeomPointers();
    void errorgrad(double* err, double* grad, double* param) override;

public:
    ConstraintC2CDistance(Circle& c1, Circle& c2, double* d);
};

// C2LDistance
class ConstraintC2LDistance: public Constraint
{
public:
    Circle circle;
    Line line;

private:
    DeriParam distance() const
    {
        return pvec[0];
    }

    // writes pointers in pvec to the parameters of c, l
    void ReconstructGeomPointers();
    void errorgrad(double* err, double* grad, double* param) override;

public:
    ConstraintC2LDistance(Circle& c, Line& l, double* d);
};

// P2CDistance
class ConstraintP2CDistance: public Constraint
{
private:
    Circle circle;
    Point pt;
    DeriParam distance() const
    {
        return pvec[0];
    }
    void ReconstructGeomPointers();  // writes pointers in pvec to the parameters of c
    void errorgrad(double* err, double* grad, double* param) override;

public:
    ConstraintP2CDistance(Point& p, Circle& c, double* d);
};

// ArcLength
class ConstraintArcLength: public Constraint
{
private:
    Arc arc;
    DeriParam distance() const
    {
        return pvec[0];
    }
    void ReconstructGeomPointers();  // writes pointers in pvec to the parameters of a
    void errorgrad(double* err, double* grad, double* param) override;

public:
    ConstraintArcLength(Arc& a, double* d);
};

}  // namespace GCS

#endif  // PLANEGCS_CONSTRAINTS_H
