/***************************************************************************
 *   Copyright (c) Konstantinos Poulios      (logari81@gmail.com) 2011     *
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

#ifndef PLANEGCS_GCS_H
#define PLANEGCS_GCS_H

#include "SubSystem.h"
#include <boost/concept_check.hpp>
#include <boost/graph/graph_concepts.hpp>

namespace GCS
{
    ///////////////////////////////////////
    // Other BFGS Solver parameters
    ///////////////////////////////////////
    #define XconvergenceRough 1e-8
    #define smallF            1e-20

    ///////////////////////////////////////
    // Solver
    ///////////////////////////////////////

    enum SolveStatus {
        Success = 0,        // Found a solution zeroing the error function
        Converged = 1,      // Found a solution minimizing the error function
        Failed = 2,         // Failed to find any solution
        SuccessfulSolutionInvalid = 3, // This is a solution where the solver succeeded, but the resulting geometry is OCE-invalid
    };

    enum Algorithm {
        BFGS = 0,
        LevenbergMarquardt = 1,
        DogLeg = 2
    };

    enum DogLegGaussStep {
        FullPivLU = 0,
        LeastNormFullPivLU = 1,
        LeastNormLdlt = 2
    };

    enum QRAlgorithm {
        EigenDenseQR = 0,
        EigenSparseQR = 1
    };

    enum DebugMode {
        NoDebug = 0,
        Minimal = 1,
        IterationLevel = 2
    };

    class System
    {
    // This is the main class. It holds all constraints and information
    // about partitioning into subsystems and solution strategies
    private:
        VEC_pD plist; // list of the unknown parameters
        VEC_pD pdrivenlist; // list of parameters of driven constraints
        MAP_pD_I pIndex;

        VEC_pD pdependentparameters; // list of dependent parameters by the system

        std::vector<Constraint *> clist;
        std::map<Constraint *,VEC_pD > c2p; // constraint to parameter adjacency list
        std::map<double *,std::vector<Constraint *> > p2c; // parameter to constraint adjacency list

        std::vector<SubSystem *> subSystems, subSystemsAux;
        void clearSubSystems();

        VEC_D reference;
        void setReference();     // copies the current parameter values to reference
        void resetToReference(); // reverts all parameter values to the stored reference

        std::vector< VEC_pD > plists;                    // partitioned plist except equality constraints
        std::vector< std::vector<Constraint *> > clists; // partitioned clist except equality constraints
        std::vector< MAP_pD_pD > reductionmaps;          // for simplification of equality constraints

        int dofs;
        std::set<Constraint *> redundant;
        VEC_I conflictingTags, redundantTags;

        bool hasUnknowns;  // if plist is filled with the unknown parameters
        bool hasDiagnosis; // if dofs, conflictingTags, redundantTags are up to date
        bool isInit;       // if plists, clists, reductionmaps are up to date

        int solve_BFGS(SubSystem *subsys, bool isFine=true, bool isRedundantsolving=false);
        int solve_LM(SubSystem *subsys, bool isRedundantsolving=false);
        int solve_DL(SubSystem *subsys, bool isRedundantsolving=false);

        void makeReducedJacobian(Eigen::MatrixXd &J, std::map<int,int> &jacobianconstraintmap, GCS::VEC_pD &pdiagnoselist, std::map< int , int> &tagmultiplicity);

        #ifdef _GCS_EXTRACT_SOLVER_SUBSYSTEM_
        void extractSubsystem(SubSystem *subsys, bool isRedundantsolving);
        #endif
    public:
        int maxIter;
        int maxIterRedundant;
        bool sketchSizeMultiplier; // if true note that the total number of iterations allowed is MaxIterations *xLength
        bool sketchSizeMultiplierRedundant;
        double convergence;
        double convergenceRedundant;
        QRAlgorithm qrAlgorithm;
        DogLegGaussStep dogLegGaussStep;
        double qrpivotThreshold;
        DebugMode debugMode;
        double LM_eps;
        double LM_eps1;
        double LM_tau;
        double DL_tolg;
        double DL_tolx;
        double DL_tolf;
        double LM_epsRedundant;
        double LM_eps1Redundant;
        double LM_tauRedundant;
        double DL_tolgRedundant;
        double DL_tolxRedundant;
        double DL_tolfRedundant;

    public:
        System();
        /*System(std::vector<Constraint *> clist_);*/
        ~System();

        void clear();
        void clearByTag(int tagId);

        int addConstraint(Constraint *constr);
        void removeConstraint(Constraint *constr);

        // basic constraints
        int addConstraintEqual(double *param1, double *param2, int tagId=0, bool driving = true);
        int addConstraintProportional(double *param1, double *param2, double ratio, int tagId, bool driving = true);
        int addConstraintDifference(double *param1, double *param2,
                                    double *difference, int tagId=0, bool driving = true);
        int addConstraintP2PDistance(Point &p1, Point &p2, double *distance, int tagId=0, bool driving = true);
        int addConstraintP2PAngle(Point &p1, Point &p2, double *angle,
                                  double incrAngle, int tagId=0, bool driving = true);
        int addConstraintP2PAngle(Point &p1, Point &p2, double *angle, int tagId=0, bool driving = true);
        int addConstraintP2LDistance(Point &p, Line &l, double *distance, int tagId=0, bool driving = true);
        int addConstraintPointOnLine(Point &p, Line &l, int tagId=0, bool driving = true);
        int addConstraintPointOnLine(Point &p, Point &lp1, Point &lp2, int tagId=0, bool driving = true);
        int addConstraintPointOnPerpBisector(Point &p, Line &l, int tagId=0, bool driving = true);
        int addConstraintPointOnPerpBisector(Point &p, Point &lp1, Point &lp2, int tagId=0, bool driving = true);
        int addConstraintParallel(Line &l1, Line &l2, int tagId=0, bool driving = true);
        int addConstraintPerpendicular(Line &l1, Line &l2, int tagId=0, bool driving = true);
        int addConstraintPerpendicular(Point &l1p1, Point &l1p2,
                                       Point &l2p1, Point &l2p2, int tagId=0, bool driving = true);
        int addConstraintL2LAngle(Line &l1, Line &l2, double *angle, int tagId=0, bool driving = true);
        int addConstraintL2LAngle(Point &l1p1, Point &l1p2, Point &l2p1, Point &l2p2,
                                  double *angle, int tagId=0, bool driving = true);
        int addConstraintAngleViaPoint(Curve &crv1, Curve &crv2, Point &p,
                                       double *angle, int tagId=0, bool driving = true);
        int addConstraintMidpointOnLine(Line &l1, Line &l2, int tagId=0, bool driving = true);
        int addConstraintMidpointOnLine(Point &l1p1, Point &l1p2, Point &l2p1, Point &l2p2,
                                        int tagId=0, bool driving = true);
        int addConstraintTangentCircumf(Point &p1, Point &p2, double *rd1, double *rd2,
                                        bool internal=false, int tagId=0, bool driving = true);

        // derived constraints
        int addConstraintP2PCoincident(Point &p1, Point &p2, int tagId=0, bool driving = true);
        int addConstraintHorizontal(Line &l, int tagId=0, bool driving = true);
        int addConstraintHorizontal(Point &p1, Point &p2, int tagId=0, bool driving = true);
        int addConstraintVertical(Line &l, int tagId=0, bool driving = true);
        int addConstraintVertical(Point &p1, Point &p2, int tagId=0, bool driving = true);
        int addConstraintCoordinateX(Point &p, double *x, int tagId=0, bool driving = true);
        int addConstraintCoordinateY(Point &p, double *y, int tagId=0, bool driving = true);
        int addConstraintArcRules(Arc &a, int tagId=0, bool driving = true);
        int addConstraintPointOnCircle(Point &p, Circle &c, int tagId=0, bool driving = true);
        int addConstraintPointOnEllipse(Point &p, Ellipse &e, int tagId=0, bool driving = true);
        int addConstraintPointOnHyperbolicArc(Point &p, ArcOfHyperbola &e, int tagId=0, bool driving = true);
        int addConstraintPointOnParabolicArc(Point &p, ArcOfParabola &e, int tagId=0, bool driving = true);
        int addConstraintArcOfEllipseRules(ArcOfEllipse &a, int tagId=0, bool driving = true);
        int addConstraintCurveValue(Point &p, Curve &a, double *u, int tagId=0, bool driving = true);
        int addConstraintArcOfHyperbolaRules(ArcOfHyperbola &a, int tagId=0, bool driving = true);
        int addConstraintArcOfParabolaRules(ArcOfParabola &a, int tagId=0, bool driving = true);
        int addConstraintPointOnArc(Point &p, Arc &a, int tagId=0, bool driving = true);
        int addConstraintPerpendicularLine2Arc(Point &p1, Point &p2, Arc &a,
                                               int tagId=0, bool driving = true);
        int addConstraintPerpendicularArc2Line(Arc &a, Point &p1, Point &p2,
                                               int tagId=0, bool driving = true);
        int addConstraintPerpendicularCircle2Arc(Point &center, double *radius, Arc &a,
                                                 int tagId=0, bool driving = true);
        int addConstraintPerpendicularArc2Circle(Arc &a, Point &center, double *radius,
                                                 int tagId=0, bool driving = true);
        int addConstraintPerpendicularArc2Arc(Arc &a1, bool reverse1,
                                              Arc &a2, bool reverse2, int tagId=0, bool driving = true);
        int addConstraintTangent(Line &l, Circle &c, int tagId=0, bool driving = true);
        int addConstraintTangent(Line &l, Ellipse &e, int tagId=0, bool driving = true);
        int addConstraintTangent(Line &l, Arc &a, int tagId=0, bool driving = true);
        int addConstraintTangent(Circle &c1, Circle &c2, int tagId=0, bool driving = true);
        int addConstraintTangent(Arc &a1, Arc &a2, int tagId=0, bool driving = true);
        int addConstraintTangent(Circle &c, Arc &a, int tagId=0, bool driving = true);

        int addConstraintCircleRadius(Circle &c, double *radius, int tagId=0, bool driving = true);
        int addConstraintArcRadius(Arc &a, double *radius, int tagId=0, bool driving = true);
        int addConstraintCircleDiameter(Circle &c, double *radius, int tagId=0, bool driving = true);
        int addConstraintArcDiameter(Arc &a, double *radius, int tagId=0, bool driving = true);
        int addConstraintEqualLength(Line &l1, Line &l2, double *length, int tagId=0, bool driving = true);
        int addConstraintEqualRadius(Circle &c1, Circle &c2, int tagId=0, bool driving = true);
        int addConstraintEqualRadii(Ellipse &e1, Ellipse &e2, int tagId=0, bool driving = true);
        int addConstraintEqualRadii(ArcOfHyperbola &a1, ArcOfHyperbola &a2, int tagId=0, bool driving = true);
        int addConstraintEqualRadius(Circle &c1, Arc &a2, int tagId=0, bool driving = true);
        int addConstraintEqualRadius(Arc &a1, Arc &a2, int tagId=0, bool driving = true);
        int addConstraintEqualFocus(ArcOfParabola &a1, ArcOfParabola &a2, int tagId=0, bool driving = true);
        int addConstraintP2PSymmetric(Point &p1, Point &p2, Line &l, int tagId=0, bool driving = true);
        int addConstraintP2PSymmetric(Point &p1, Point &p2, Point &p, int tagId=0, bool driving = true);
        int addConstraintSnellsLaw(Curve &ray1, Curve &ray2,
                                   Curve &boundary, Point p,
                                   double* n1, double* n2,
                                   bool flipn1, bool flipn2,
                                   int tagId, bool driving = true);

        // internal alignment constraints
        int addConstraintInternalAlignmentPoint2Ellipse(Ellipse &e, Point &p1, InternalAlignmentType alignmentType, int tagId=0, bool driving = true);
        int addConstraintInternalAlignmentEllipseMajorDiameter(Ellipse &e, Point &p1, Point &p2, int tagId=0, bool driving = true);
        int addConstraintInternalAlignmentEllipseMinorDiameter(Ellipse &e, Point &p1, Point &p2, int tagId=0, bool driving = true);
        int addConstraintInternalAlignmentEllipseFocus1(Ellipse &e, Point &p1, int tagId=0, bool driving = true);
        int addConstraintInternalAlignmentEllipseFocus2(Ellipse &e, Point &p1, int tagId=0, bool driving = true);
        int addConstraintInternalAlignmentPoint2Hyperbola(Hyperbola &e, Point &p1, InternalAlignmentType alignmentType, int tagId=0, bool driving = true);
        int addConstraintInternalAlignmentHyperbolaMajorDiameter(Hyperbola &e, Point &p1, Point &p2, int tagId=0, bool driving = true);
        int addConstraintInternalAlignmentHyperbolaMinorDiameter(Hyperbola &e, Point &p1, Point &p2, int tagId=0, bool driving = true);
        int addConstraintInternalAlignmentHyperbolaFocus(Hyperbola &e, Point &p1, int tagId=0, bool driving = true);
        int addConstraintInternalAlignmentParabolaFocus(Parabola &e, Point &p1, int tagId=0, bool driving = true);
        int addConstraintInternalAlignmentBSplineControlPoint(BSpline &b, Circle &c, int poleindex, int tag=0, bool driving = true);

        double calculateAngleViaPoint(Curve &crv1, Curve &crv2, Point &p);
        double calculateAngleViaPoint(Curve &crv1, Curve &crv2, Point &p1, Point &p2);
        void calculateNormalAtPoint(Curve &crv, Point &p, double &rtnX, double &rtnY);

        // Calculates errors of all constraints which have a tag equal to
        // the one supplied. Individual errors are summed up using RMS.
        // If none are found, NAN is returned
        // If there's only one, a signed value is returned.
        // Effectively, it calculates the error of a UI constraint
        double calculateConstraintErrorByTag(int tagId);

        void rescaleConstraint(int id, double coeff);

        void declareUnknowns(VEC_pD &params);
        void declareDrivenParams(VEC_pD &params);
        void initSolution(Algorithm alg=DogLeg);

        int solve(bool isFine=true, Algorithm alg=DogLeg, bool isRedundantsolving=false);
        int solve(VEC_pD &params, bool isFine=true, Algorithm alg=DogLeg, bool isRedundantsolving=false);
        int solve(SubSystem *subsys, bool isFine=true, Algorithm alg=DogLeg, bool isRedundantsolving=false);
        int solve(SubSystem *subsysA, SubSystem *subsysB, bool isFine=true, bool isRedundantsolving=false);

        void applySolution();
        void undoSolution();
        //FIXME: looks like XconvergenceFine is not the solver precision, at least in DogLeg solver.
        // Note: Yes, every solver has a different way of interpreting precision
        // but one has to study what is this needed for in order to decide
        // what to return (this is unchanged from previous versions)
        double getFinePrecision(){ return convergence;}

        int diagnose(Algorithm alg=DogLeg);
        int dofsNumber() const { return hasDiagnosis ? dofs : -1; }
        void getConflicting(VEC_I &conflictingOut) const
          { conflictingOut = hasDiagnosis ? conflictingTags : VEC_I(0); }
        void getRedundant(VEC_I &redundantOut) const
          { redundantOut = hasDiagnosis ? redundantTags : VEC_I(0); }
        void getDependentParams(VEC_pD &pconstraintplistOut) const
          { pconstraintplistOut = pdependentparameters;}
    };


    ///////////////////////////////////////
    // Helper elements
    ///////////////////////////////////////

    void free(VEC_pD &doublevec);
    void free(std::vector<Constraint *> &constrvec);
    void free(std::vector<SubSystem *> &subsysvec);

} //namespace GCS

#endif // PLANEGCS_GCS_H
