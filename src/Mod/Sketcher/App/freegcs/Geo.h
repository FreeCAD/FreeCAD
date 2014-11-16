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

#ifndef FREEGCS_GEO_H
#define FREEGCS_GEO_H

#include <cmath>
#include "Util.h"

namespace GCS
{
    class Vector2D /* DeepSOIC: I tried to reuse Base::Vector2D by #include <Base/Tools2D.h>,
                    * but I failed to resolve bullshit compilation errors that arose in the process, so...
                    * Anyway, the benefit is that solver has less dependencies on FreeCAD and can be
                    * stripped off easier.
                    * I could have used Eigen's Vector2f, but I found it overblown and too complex to use.
                    */
    {
    public:
        Vector2D(){x=0; y=0;}
        Vector2D(double x, double y) {this->x = x; this->y = y;}
        double x;
        double y;
        double length() {return sqrt(x*x + y*y);}

        //unlike other vectors in FreeCAD, this normalization creates a new vector instead of modifying existing one.
        Vector2D getNormalized(){double l=length(); if(l==0.0) l=1.0; return Vector2D(x/l,y/l);} //returns zero vector if the original is zero.
    };

    ///////////////////////////////////////
    // Geometries
    ///////////////////////////////////////
    class Point
    {
    public:
        Point(){x = 0; y = 0;}
        double *x;
        double *y;
    };

    class Curve //a base class for all curve-based objects (line, circle/arc, ellipse/arc)
    {
    public:

        //returns normal vector. The vector should point to the left when one
        // walks along the curve from start to end. Ellipses and circles are
        // assumed to be walked counterclockwise, so the vector should point
        // into the shape.
        //derivparam is a pointer to a curve parameter (or point coordinate) to
        // compute the derivative for. if derivparam is nullptr, the actual
        // normal vector is returned, otherwise a derivative of normal vector by
        // *derivparam is returned.
        virtual Vector2D CalculateNormal(Point &p, double* derivparam = 0) = 0;

        //adds curve's parameters to pvec (used by constraints)
        virtual int PushOwnParams(VEC_pD &pvec) = 0;
        //recunstruct curve's parameters reading them from pvec starting from index cnt.
        //cnt will be incremented by the same value as returned by PushOwnParams()
        virtual void ReconstructOnNewPvec (VEC_pD &pvec, int &cnt) = 0;
        virtual Curve* Copy() = 0; //DeepSOIC: I haven't found a way to simply copy a curve object provided pointer to a curve object.
    };

    class Line: public Curve
    {
    public:
        Line(){}
        Point p1;
        Point p2;
        Vector2D CalculateNormal(Point &p, double* derivparam = 0);
        virtual int PushOwnParams(VEC_pD &pvec);
        virtual void ReconstructOnNewPvec (VEC_pD &pvec, int &cnt);
        virtual Line* Copy();
    };

    class Circle: public Curve
    {
    public:
        Circle(){rad = 0;}
        Point center;
        double *rad;
        Vector2D CalculateNormal(Point &p, double* derivparam = 0);
        virtual int PushOwnParams(VEC_pD &pvec);
        virtual void ReconstructOnNewPvec (VEC_pD &pvec, int &cnt);
        virtual Circle* Copy();
    };

    class Arc: public Circle
    {
    public:
        Arc(){startAngle=0;endAngle=0;rad=0;}
        double *startAngle;
        double *endAngle;
        //double *rad; //inherited
        Point start;
        Point end;
        //Point center; //inherited
        virtual int PushOwnParams(VEC_pD &pvec);
        virtual void ReconstructOnNewPvec (VEC_pD &pvec, int &cnt);
        virtual Arc* Copy();
    };
    
    class Ellipse: public Curve
    {
    public:
        Ellipse(){ radmin = 0;}
        Point center; 
        double *focus1X;
        double *focus1Y;
        double *radmin;
        Vector2D CalculateNormal(Point &p, double* derivparam = 0);
        virtual int PushOwnParams(VEC_pD &pvec);
        virtual void ReconstructOnNewPvec (VEC_pD &pvec, int &cnt);
        virtual Ellipse* Copy();
    };
    
    class ArcOfEllipse: public Ellipse
    {
    public:
        ArcOfEllipse(){startAngle=0;endAngle=0;radmin = 0;}
        double *startAngle;
        double *endAngle;
        //double *radmin; //inherited
        Point start;
        Point end;
        //Point center;  //inherited
        //double *focus1X; //inherited
        //double *focus1Y; //inherited
        virtual int PushOwnParams(VEC_pD &pvec);
        virtual void ReconstructOnNewPvec (VEC_pD &pvec, int &cnt);
        virtual ArcOfEllipse* Copy();
    };

} //namespace GCS

#endif // FREEGCS_GEO_H
