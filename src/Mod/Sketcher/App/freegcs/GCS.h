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

#ifndef FREEGCS_GCS_H
#define FREEGCS_GCS_H

#include "SubSystem.h"

namespace GCS
{

    ///////////////////////////////////////
    // Solver
    ///////////////////////////////////////

    enum SolveStatus {
        Success = 0,   // Found a solution zeroing the error function
        Converged = 1, // Found a solution minimizing the error function
        Failed = 2     // Failed to find any solution
    };

    enum Algorithm {
        BFGS = 0,
        LevenbergMarquardt = 1,
        DogLeg = 2
    };

    class System
    {
    // This is the main class. It holds all constraints and information
    // about partitioning into subsystems and solution strategies
    private:
        std::vector<Constraint *> clist;

        std::map<Constraint *,VEC_pD > c2p; // constraint to parameter adjacency list
        std::map<double *,std::vector<Constraint *> > p2c; // parameter to constraint adjacency list

        SubSystem *subsys0; // has the highest priority, always used as the primary subsystem
        SubSystem *subsys1; // normally used as secondary subsystem, it is considered primary only if subsys0 is missing
        SubSystem *subsys2; // has the lowest priority, always used as secondary system
        void clearSubSystems();

        MAP_pD_D reference;
        void clearReference();
        void resetToReference();

        MAP_pD_pD reductionmap; // for simplification of equality constraints

        bool init;

        int solve_BFGS(SubSystem *subsys, bool isFine);
        int solve_LM(SubSystem *subsys);
        int solve_DL(SubSystem *subsys);
    public:
        System();
        System(std::vector<Constraint *> clist_);
        ~System();

        void clear();
        void clearByTag(int tagId);

        int addConstraint(Constraint *constr);
        void removeConstraint(Constraint *constr);

        // basic constraints
        int addConstraintEqual(double *param1, double *param2, int tagId=0);
        int addConstraintDifference(double *param1, double *param2,
                                    double *difference, int tagId=0);
        int addConstraintP2PDistance(Point &p1, Point &p2, double *distance, int tagId=0);
        int addConstraintP2PAngle(Point &p1, Point &p2, double *angle,
                                  double incr_angle, int tagId=0);
        int addConstraintP2PAngle(Point &p1, Point &p2, double *angle, int tagId=0);
        int addConstraintP2LDistance(Point &p, Line &l, double *distance, int tagId=0);
        int addConstraintPointOnLine(Point &p, Line &l, int tagId=0);
        int addConstraintParallel(Line &l1, Line &l2, int tagId=0);
        int addConstraintPerpendicular(Line &l1, Line &l2, int tagId=0);
        int addConstraintPerpendicular(Point &l1p1, Point &l1p2,
                                       Point &l2p1, Point &l2p2, int tagId=0);
        int addConstraintL2LAngle(Line &l1, Line &l2, double *angle, int tagId=0);
        int addConstraintL2LAngle(Point &l1p1, Point &l1p2, Point &l2p1, Point &l2p2,
                                  double *angle, int tagId=0);
        int addConstraintMidpointOnLine(Line &l1, Line &l2, int tagId=0);
        int addConstraintMidpointOnLine(Point &l1p1, Point &l1p2, Point &l2p1, Point &l2p2,
                                        int tagId=0);
        int addConstraintTangentCircumf(Point &p1, Point &p2, double *rad1, double *rad2,
                                        bool internal=false, int tagId=0);

        // derived constraints
        int addConstraintP2PCoincident(Point &p1, Point &p2, int tagId=0);
        int addConstraintHorizontal(Line &l, int tagId=0);
        int addConstraintHorizontal(Point &p1, Point &p2, int tagId=0);
        int addConstraintVertical(Line &l, int tagId=0);
        int addConstraintVertical(Point &p1, Point &p2, int tagId=0);
        int addConstraintCoordinateX(Point &p, double *x, int tagId=0);
        int addConstraintCoordinateY(Point &p, double *y, int tagId=0);
        int addConstraintArcRules(Arc &a, int tagId=0);
        int addConstraintPointOnCircle(Point &p, Circle &c, int tagId=0);
        int addConstraintPointOnArc(Point &p, Arc &a, int tagId=0);
        int addConstraintTangent(Line &l, Circle &c, int tagId=0);
        int addConstraintTangent(Line &l, Arc &a, int tagId=0);
        int addConstraintTangent(Circle &c1, Circle &c2, int tagId=0);
        int addConstraintTangent(Arc &a1, Arc &a2, int tagId=0);
        int addConstraintTangent(Circle &c, Arc &a, int tagId=0);
        int addConstraintTangentLine2Arc(Point &p1, Point &p2, Arc &a, int tagId=0);
        int addConstraintTangentArc2Line(Arc &a, Point &p1, Point &p2, int tagId=0);
        int addConstraintTangentCircle2Arc(Circle &c, Arc &a, int tagId=0);
        int addConstraintTangentArc2Circle(Arc &a, Circle &c, int tagId=0);
        int addConstraintTangentArc2Arc(Arc &a1, bool reverse1, Arc &a2, bool reverse2,
                                        int tagId=0);
        int addConstraintCircleRadius(Circle &c, double *radius, int tagId=0);
        int addConstraintArcRadius(Arc &a, double *radius, int tagId=0);
        int addConstraintEqualLength(Line &l1, Line &l2, double *length, int tagId=0);
        int addConstraintEqualRadius(Circle &c1, Circle &c2, int tagId=0);
        int addConstraintEqualRadius(Circle &c1, Arc &a2, int tagId=0);
        int addConstraintEqualRadius(Arc &a1, Arc &a2, int tagId=0);
        int addConstraintP2PSymmetric(Point &p1, Point &p2, Line &l, int tagId=0);
        void rescaleConstraint(int id, double coeff);

        void initSolution(VEC_pD &params);

        int solve(bool isFine=true, Algorithm alg=DogLeg);
        int solve(VEC_pD &params, bool isFine=true, Algorithm alg=DogLeg);
        int solve(SubSystem *subsys, bool isFine=true, Algorithm alg=DogLeg);
        int solve(SubSystem *subsysA, SubSystem *subsysB, bool isFine=true);

        void getSubSystems(std::vector<SubSystem *> &subsysvec);
        void applySolution();
        void undoSolution();

        bool isInit() const { return init; }

        int diagnose(VEC_pD &params, VEC_I &conflicting);
    };

    ///////////////////////////////////////
    // BFGS Solver parameters
    ///////////////////////////////////////
    #define XconvergenceRough 1e-8
    #define XconvergenceFine  1e-10
    #define smallF            1e-20
    #define MaxIterations     100 //Note that the total number of iterations allowed is MaxIterations *xLength

    ///////////////////////////////////////
    // Helper elements
    ///////////////////////////////////////

    void free(VEC_pD &doublevec);
    void free(std::vector<Constraint *> &constrvec);
    void free(std::vector<SubSystem *> &subsysvec);

} //namespace GCS

#endif // FREEGCS_GCS_H
