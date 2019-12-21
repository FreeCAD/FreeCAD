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

#include "Geo.h"
#include "Util.h"
#include <boost/graph/graph_concepts.hpp>

//#define _GCS_EXTRACT_SOLVER_SUBSYSTEM_ // This enables debugging code intended to extract information to file bug reports against Eigen, not for production code

#ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
#define _PROTECTED_UNLESS_EXTRACT_MODE_ public
#else
#define _PROTECTED_UNLESS_EXTRACT_MODE_ protected
#endif

namespace GCS
{

    ///////////////////////////////////////
    // Constraints
    ///////////////////////////////////////

    enum ConstraintType {
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
        EqualFocalDistance = 24
    };
    
    enum InternalAlignmentType {
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

    class Constraint
    {
    _PROTECTED_UNLESS_EXTRACT_MODE_:
        VEC_pD origpvec; // is used only as a reference for redirecting and reverting pvec
        VEC_pD pvec;
        double scale;
        int tag;
        bool pvecChangedFlag;  //indicates that pvec has changed and saved pointers must be reconstructed (currently used only in AngleViaPoint)
        bool driving;
    public:
        Constraint();
        virtual ~Constraint(){}

        inline VEC_pD params() { return pvec; }

        void redirectParams(MAP_pD_pD redirectionmap);
        void revertParams();
        void setTag(int tagId) { tag = tagId; }
        int getTag() { return tag; }
        
        void setDriving(bool isdriving) { driving = isdriving; }
        bool isDriving() const { return driving; }

        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
        // virtual void grad(MAP_pD_D &deriv);  --> TODO: vectorized grad version
        virtual double maxStep(MAP_pD_D &dir, double lim=1.);
        // Finds first occurrence of param in pvec. This is useful to test if a constraint depends 
        // on the parameter (it may not actually depend on it, e.g. angle-via-point doesn't depend 
        // on ellipse's b (radmin), but b will be included within the constraint anyway. 
        // Returns -1 if not found.
        int findParamInPvec(double* param); 
    };

    // Equal
    class ConstraintEqual : public Constraint
    {
    private:
        double ratio;
        inline double* param1() { return pvec[0]; }
        inline double* param2() { return pvec[1]; }
    public:
        ConstraintEqual(double *p1, double *p2, double p1p2ratio=1.0);
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };

    // Difference
    class ConstraintDifference : public Constraint
    {
    private:
        inline double* param1() { return pvec[0]; }
        inline double* param2() { return pvec[1]; }
        inline double* difference() { return pvec[2]; }
    public:
        ConstraintDifference(double *p1, double *p2, double *d);
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };

    // P2PDistance
    class ConstraintP2PDistance : public Constraint
    {
    private:
        inline double* p1x() { return pvec[0]; }
        inline double* p1y() { return pvec[1]; }
        inline double* p2x() { return pvec[2]; }
        inline double* p2y() { return pvec[3]; }
        inline double* distance() { return pvec[4]; }
    public:
        ConstraintP2PDistance(Point &p1, Point &p2, double *d);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintP2PDistance(){}
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
        virtual double maxStep(MAP_pD_D &dir, double lim=1.);
    };

    // P2PAngle
    class ConstraintP2PAngle : public Constraint
    {
    private:
        inline double* p1x() { return pvec[0]; }
        inline double* p1y() { return pvec[1]; }
        inline double* p2x() { return pvec[2]; }
        inline double* p2y() { return pvec[3]; }
        inline double* angle() { return pvec[4]; }
        double da;
    public:
        ConstraintP2PAngle(Point &p1, Point &p2, double *a, double da_=0.);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintP2PAngle(){}
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
        virtual double maxStep(MAP_pD_D &dir, double lim=1.);
    };

    // P2LDistance
    class ConstraintP2LDistance : public Constraint
    {
    private:
        inline double* p0x() { return pvec[0]; }
        inline double* p0y() { return pvec[1]; }
        inline double* p1x() { return pvec[2]; }
        inline double* p1y() { return pvec[3]; }
        inline double* p2x() { return pvec[4]; }
        inline double* p2y() { return pvec[5]; }
        inline double* distance() { return pvec[6]; }
    public:
        ConstraintP2LDistance(Point &p, Line &l, double *d);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintP2LDistance(){}
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
        virtual double maxStep(MAP_pD_D &dir, double lim=1.);
        double abs(double darea);
    };

    // PointOnLine
    class ConstraintPointOnLine : public Constraint
    {
    private:
        inline double* p0x() { return pvec[0]; }
        inline double* p0y() { return pvec[1]; }
        inline double* p1x() { return pvec[2]; }
        inline double* p1y() { return pvec[3]; }
        inline double* p2x() { return pvec[4]; }
        inline double* p2y() { return pvec[5]; }
    public:
        ConstraintPointOnLine(Point &p, Line &l);
        ConstraintPointOnLine(Point &p, Point &lp1, Point &lp2);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintPointOnLine(){}
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };

    // PointOnPerpBisector
    class ConstraintPointOnPerpBisector : public Constraint
    {
    private:
        inline double* p0x() { return pvec[0]; }
        inline double* p0y() { return pvec[1]; }
        inline double* p1x() { return pvec[2]; }
        inline double* p1y() { return pvec[3]; }
        inline double* p2x() { return pvec[4]; }
        inline double* p2y() { return pvec[5]; }
        void errorgrad(double *err, double *grad, double *param);
    public:
        ConstraintPointOnPerpBisector(Point &p, Line &l);
        ConstraintPointOnPerpBisector(Point &p, Point &lp1, Point &lp2);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintPointOnPerpBisector(){};
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);

        virtual double error();
        virtual double grad(double *);
    };

    // Parallel
    class ConstraintParallel : public Constraint
    {
    private:
        inline double* l1p1x() { return pvec[0]; }
        inline double* l1p1y() { return pvec[1]; }
        inline double* l1p2x() { return pvec[2]; }
        inline double* l1p2y() { return pvec[3]; }
        inline double* l2p1x() { return pvec[4]; }
        inline double* l2p1y() { return pvec[5]; }
        inline double* l2p2x() { return pvec[6]; }
        inline double* l2p2y() { return pvec[7]; }
    public:
        ConstraintParallel(Line &l1, Line &l2);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintParallel(){}
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };

    // Perpendicular
    class ConstraintPerpendicular : public Constraint
    {
    private:
        inline double* l1p1x() { return pvec[0]; }
        inline double* l1p1y() { return pvec[1]; }
        inline double* l1p2x() { return pvec[2]; }
        inline double* l1p2y() { return pvec[3]; }
        inline double* l2p1x() { return pvec[4]; }
        inline double* l2p1y() { return pvec[5]; }
        inline double* l2p2x() { return pvec[6]; }
        inline double* l2p2y() { return pvec[7]; }
    public:
        ConstraintPerpendicular(Line &l1, Line &l2);
        ConstraintPerpendicular(Point &l1p1, Point &l1p2, Point &l2p1, Point &l2p2);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintPerpendicular(){}
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };

    // L2LAngle
    class ConstraintL2LAngle : public Constraint
    {
    private:
        inline double* l1p1x() { return pvec[0]; }
        inline double* l1p1y() { return pvec[1]; }
        inline double* l1p2x() { return pvec[2]; }
        inline double* l1p2y() { return pvec[3]; }
        inline double* l2p1x() { return pvec[4]; }
        inline double* l2p1y() { return pvec[5]; }
        inline double* l2p2x() { return pvec[6]; }
        inline double* l2p2y() { return pvec[7]; }
        inline double* angle() { return pvec[8]; }
    public:
        ConstraintL2LAngle(Line &l1, Line &l2, double *a);
        ConstraintL2LAngle(Point &l1p1, Point &l1p2,
                           Point &l2p1, Point &l2p2, double *a);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintL2LAngle(){}
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
        virtual double maxStep(MAP_pD_D &dir, double lim=1.);
    };

    // MidpointOnLine
    class ConstraintMidpointOnLine : public Constraint
    {
    private:
        inline double* l1p1x() { return pvec[0]; }
        inline double* l1p1y() { return pvec[1]; }
        inline double* l1p2x() { return pvec[2]; }
        inline double* l1p2y() { return pvec[3]; }
        inline double* l2p1x() { return pvec[4]; }
        inline double* l2p1y() { return pvec[5]; }
        inline double* l2p2x() { return pvec[6]; }
        inline double* l2p2y() { return pvec[7]; }
    public:
        ConstraintMidpointOnLine(Line &l1, Line &l2);
        ConstraintMidpointOnLine(Point &l1p1, Point &l1p2, Point &l2p1, Point &l2p2);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintMidpointOnLine(){}
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };

    // TangentCircumf
    class ConstraintTangentCircumf : public Constraint
    {
    private:
        inline double* c1x() { return pvec[0]; }
        inline double* c1y() { return pvec[1]; }
        inline double* c2x() { return pvec[2]; }
        inline double* c2y() { return pvec[3]; }
        inline double* r1() { return pvec[4]; }
        inline double* r2() { return pvec[5]; }
        bool internal;
    public:
        ConstraintTangentCircumf(Point &p1, Point &p2,
                                 double *rd1, double *rd2, bool internal_=false);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintTangentCircumf(bool internal_){internal=internal_;}
        #endif
        inline bool getInternal() {return internal;};
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };
    // PointOnEllipse
    class ConstraintPointOnEllipse : public Constraint
    {
    private:
        inline double* p1x() { return pvec[0]; }
        inline double* p1y() { return pvec[1]; }
        inline double* cx() { return pvec[2]; }
        inline double* cy() { return pvec[3]; }
        inline double* f1x() { return pvec[4]; }
        inline double* f1y() { return pvec[5]; }
        inline double* rmin() { return pvec[6]; }
    public:
        ConstraintPointOnEllipse(Point &p, Ellipse &e);
        ConstraintPointOnEllipse(Point &p, ArcOfEllipse &a);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintPointOnEllipse(){}
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };
    
    class ConstraintEllipseTangentLine : public Constraint
    {
    private:
        Line l;
        Ellipse e;
        void ReconstructGeomPointers(); //writes pointers in pvec to the parameters of crv1, crv2 and poa
        void errorgrad(double* err, double* grad, double *param); //error and gradient combined. Values are returned through pointers.
    public:
        ConstraintEllipseTangentLine(Line &l, Ellipse &e);
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };
        
    class ConstraintInternalAlignmentPoint2Ellipse : public Constraint
    {
    public:
        ConstraintInternalAlignmentPoint2Ellipse(Ellipse &e, Point &p1, InternalAlignmentType alignmentType);
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    private:
        void errorgrad(double* err, double* grad, double *param); //error and gradient combined. Values are returned through pointers.
        void ReconstructGeomPointers(); //writes pointers in pvec to the parameters of crv1, crv2 and poa
        Ellipse e;
        Point p;
        InternalAlignmentType AlignmentType;
    };

    class ConstraintInternalAlignmentPoint2Hyperbola : public Constraint
    {
    public:
        ConstraintInternalAlignmentPoint2Hyperbola(Hyperbola &e, Point &p1, InternalAlignmentType alignmentType);
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    private:
        void errorgrad(double* err, double* grad, double *param); //error and gradient combined. Values are returned through pointers.
        void ReconstructGeomPointers(); //writes pointers in pvec to the parameters of crv1, crv2 and poa
        Hyperbola e;
        Point p;
        InternalAlignmentType AlignmentType;
    };

    class ConstraintEqualMajorAxesConic : public Constraint
    {
    private:
        MajorRadiusConic * e1;
        MajorRadiusConic * e2;
        void ReconstructGeomPointers(); //writes pointers in pvec to the parameters of crv1, crv2 and poa
        void errorgrad(double* err, double* grad, double *param); //error and gradient combined. Values are returned through pointers.
    public:
        ConstraintEqualMajorAxesConic(MajorRadiusConic * a1, MajorRadiusConic * a2);
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };

    class ConstraintEqualFocalDistance : public Constraint
    {
    private:
        ArcOfParabola * e1;
        ArcOfParabola * e2;
        void ReconstructGeomPointers(); //writes pointers in pvec to the parameters of crv1, crv2 and poa
        void errorgrad(double* err, double* grad, double *param); //error and gradient combined. Values are returned through pointers.
    public:
        ConstraintEqualFocalDistance(ArcOfParabola * a1, ArcOfParabola * a2);
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };

    class ConstraintCurveValue : public Constraint
    {
    private:
        inline double* pcoord() { return pvec[2]; } //defines, which coordinate of point is being constrained by this constraint
        inline double* u() { return pvec[3]; }
        void errorgrad(double* err, double* grad, double *param); //error and gradient combined. Values are returned through pointers.
        void ReconstructGeomPointers(); //writes pointers in pvec to the parameters of crv1, crv2 and poa
        Curve* crv;
        Point p;
    public:
        /**
         * @brief ConstraintCurveValue: solver constraint that ties parameter value with point coordinates, according to curve's parametric equation.
         * @param p : endpoint to be constrained
         * @param pcoord : pointer to point coordinate to be constrained. Must be either p.x or p.y
         * @param crv : the curve (crv->Value() must be functional)
         * @param u : pointer to u parameter corresponding to the point
         */
        ConstraintCurveValue(Point &p, double* pcoord, Curve& crv, double* u);
        ~ConstraintCurveValue();
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
        virtual double maxStep(MAP_pD_D &dir, double lim=1.);
    };
    
    // PointOnHyperbola
    class ConstraintPointOnHyperbola : public Constraint
    {
    private:
        inline double* p1x() { return pvec[0]; }
        inline double* p1y() { return pvec[1]; }
        inline double* cx() { return pvec[2]; }
        inline double* cy() { return pvec[3]; }
        inline double* f1x() { return pvec[4]; }
        inline double* f1y() { return pvec[5]; }
        inline double* rmin() { return pvec[6]; }
    public:
        ConstraintPointOnHyperbola(Point &p, Hyperbola &e);
        ConstraintPointOnHyperbola(Point &p, ArcOfHyperbola &a);
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintPointOnHyperbola(){}
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };

    // PointOnParabola
    class ConstraintPointOnParabola : public Constraint
    {
    private:
        void errorgrad(double* err, double* grad, double *param); //error and gradient combined. Values are returned through pointers.
        void ReconstructGeomPointers(); //writes pointers in pvec to the parameters of crv1, crv2 and poa
        Parabola* parab;
        Point p;
    public:
        ConstraintPointOnParabola(Point &p, Parabola &e);
        ConstraintPointOnParabola(Point &p, ArcOfParabola &a);
	~ConstraintPointOnParabola();
        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        inline ConstraintPointOnParabola(){}
        #endif
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };
    
    class ConstraintAngleViaPoint : public Constraint
    {
    private:
        inline double* angle() { return pvec[0]; };
        Curve* crv1;
        Curve* crv2;
        //These two pointers hold copies of the curves that were passed on
        // constraint creation. The curves must be deleted upon destruction of
        // the constraint. It is necessary to have copies, since messing with
        // original objects that were passed is a very bad idea (but messing is
        // necessary, because we need to support redirectParams()/revertParams
        // functions.
        //The pointers in the curves need to be reconstructed if pvec was redirected
        // (test pvecChangedFlag variable before use!)
        Point poa;//poa=point of angle //needs to be reconstructed if pvec was redirected/reverted. The point is easily shallow-copied by C++, so no pointer type here and no delete is necessary.
        void ReconstructGeomPointers(); //writes pointers in pvec to the parameters of crv1, crv2 and poa
    public:
        ConstraintAngleViaPoint(Curve &acrv1, Curve &acrv2, Point p, double* angle);
        ~ConstraintAngleViaPoint();
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };

    class ConstraintSnell : public Constraint //snell's law angles constrainer. Point needs to lie on all three curves to be constraied.
    {
    private:
        inline double* n1() { return pvec[0]; };
        inline double* n2() { return pvec[1]; };
        Curve* ray1;
        Curve* ray2;
        Curve* boundary;
        //These pointers hold copies of the curves that were passed on
        // constraint creation. The curves must be deleted upon destruction of
        // the constraint. It is necessary to have copies, since messing with
        // original objects that were passed is a very bad idea (but messing is
        // necessary, because we need to support redirectParams()/revertParams
        // functions.
        //The pointers in the curves need to be reconstructed if pvec was redirected
        // (test pvecChangedFlag variable before use!)
        Point poa;//poa=point of refraction //needs to be reconstructed if pvec was redirected/reverted. The point is easily shallow-copied by C++, so no pointer type here and no delete is necessary.
        bool flipn1, flipn2;
        void ReconstructGeomPointers(); //writes pointers in pvec to the parameters of crv1, crv2 and poa
        void errorgrad(double* err, double* grad, double *param); //error and gradient combined. Values are returned through pointers.
    public:
        //n1dn2 = n1 divided by n2. from n1 to n2. flipn1 = true instructs to flip ray1's tangent
        ConstraintSnell(Curve &ray1, Curve &ray2, Curve &boundary, Point p, double* n1, double* n2, bool flipn1, bool flipn2);
        ~ConstraintSnell();
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };


} //namespace GCS

#endif // PLANEGCS_CONSTRAINTS_H
