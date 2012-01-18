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

#ifndef FREEGCS_CONSTRAINTS_H
#define FREEGCS_CONSTRAINTS_H

#include "Geo.h"
#include "Util.h"
#include <Eigen/Dense>

namespace GCS
{

    ///////////////////////////////////////
    // Constraints
    ///////////////////////////////////////

    enum ConstraintType {
        None = 0,
	ASParallel,
	ASDistance
    };
    
    enum ParallelType {
      
      NormalWhatever = 0,
      NormalSame,
      NormalOpposite
    };

    class Constraint
    {
    protected:
        VEC_pD origpvec; // is used only as a reference for redirecting and reverting pvec
        VEC_pD pvec;
        double scale;
        int tag;
    public:
        Constraint();

        inline VEC_pD params() { return pvec; }

        void redirectParams(MAP_pD_pD redirectionmap);
        void revertParams();
        void setTag(int tagId) { tag = tagId; }
        int getTag() { return tag; }

        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
        // virtual void grad(MAP_pD_D &deriv);  --> TODO: vectorized grad version
        virtual double maxStep(MAP_pD_D &dir, double lim=1.);
    };
    
        // AS_plane_parallel
    class ConstraintParralelFaceAS : public Constraint
    {
    private:
        inline double* q0a() { return pvec[0]; }
        inline double* q0b() { return pvec[1]; }
        inline double* q0c() { return pvec[2]; }
        inline double* q0d() { return pvec[3]; }
        inline double* q1a() { return pvec[4]; }
        inline double* q1b() { return pvec[5]; }
        inline double* q1c() { return pvec[6]; }
        inline double* q1d() { return pvec[7]; }

        Eigen::Vector3d n0, n1;
	ParallelType *type;
	
    public:
	ConstraintParralelFaceAS(GCS::Solid& s0, GCS::Solid& s1, ParallelType *t);
	
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };
    
    class ConstraintFaceDistanceAS : public Constraint
    {
    private:
        inline double* p0x() { return pvec[0]; }
        inline double* p0y() { return pvec[1]; }
        inline double* p0z() { return pvec[2]; }
        inline double* p1x() { return pvec[3]; }
        inline double* p1y() { return pvec[4]; }
        inline double* p1z() { return pvec[5]; }
        inline double* q0a() { return pvec[6]; }
        inline double* q0b() { return pvec[7]; }
        inline double* q0c() { return pvec[8]; }
        inline double* q0d() { return pvec[9]; }
        inline double* q1a() { return pvec[10]; }
        inline double* q1b() { return pvec[11]; }
        inline double* q1c() { return pvec[12]; }
        inline double* q1d() { return pvec[13]; }

        Eigen::Vector3d p0, p1;
	Eigen::Vector3d n0;
	
	double *dist;
    public:
	ConstraintFaceDistanceAS(GCS::Solid& s0, GCS::Solid& s1, double *d);
        virtual ConstraintType getTypeId();
        virtual void rescale(double coef=1.);
        virtual double error();
        virtual double grad(double *);
    };


} //namespace GCS

#endif // FREEGCS_CONSTRAINTS_H
